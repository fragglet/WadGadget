//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "fs/vfile.h"
#include "conv/error.h"
#include "conv/graphic.h"
#include "conv/vpng.h"

// Hexen loading screen
#define HIRES_SCREEN_W  640
#define HIRES_SCREEN_H  480
#define HIRES_MIN_LENGTH  ((HIRES_SCREEN_W * HIRES_SCREEN_H / 2) + (16 * 3))

// Heretic/Hexen raw fullscreen images.
#define FULLSCREEN_W  320
#define FULLSCREEN_H  200
#define FULLSCREEN_SZ (320 * 200)

#define MAX_POST_LEN  0x80

void V_SwapPatchHeader(struct patch_header *hdr)
{
	SwapLE16(&hdr->width);
	SwapLE16(&hdr->height);
	SwapLE16(&hdr->leftoffset);
	SwapLE16(&hdr->topoffset);
};

static void SwapOffsetsChunk(struct offsets_chunk *chunk)
{
	SwapBE32(&chunk->leftoffset);
	SwapBE32(&chunk->topoffset);
}

static void WriteOffsetChunk(png_structp ppng, const struct patch_header *hdr)
{
	struct offsets_chunk chunk;

	chunk.leftoffset = hdr->leftoffset;
	chunk.topoffset = hdr->topoffset;
	SwapOffsetsChunk(&chunk);
	png_write_chunk(ppng, (png_const_bytep) OFFSET_CHUNK_NAME,
	                (png_const_bytep) &chunk,
	                sizeof(chunk));
}

static VFILE *RGBABufferToPatch(uint8_t *buffer, size_t rowstep,
                                struct patch_header *hdr)
{
	VFILE *result;
	struct patch_header swapped_hdr;
	uint32_t *column_offsets;
	uint8_t *palettized, *post, alpha;
	int x, y, post_len;

	palettized = V_PalettizeRGBABuffer(doom_palette, buffer, rowstep,
	                                   hdr->width, hdr->height);

	result = vfopenmem(NULL, 0);

	// Write header.
	swapped_hdr = *hdr;
	V_SwapPatchHeader(&swapped_hdr);
	vfwrite(&swapped_hdr, 8, 1, result);

	// Write fake column directory; we'll go back later
	// and overwrite it with the actual data.
	column_offsets = checked_calloc(hdr->width, sizeof(uint32_t));
	vfwrite(column_offsets, sizeof(uint32_t), hdr->width, result);

	post = checked_calloc(MAX_POST_LEN + 4, 1);
	for (x = 0; x < hdr->width; x++) {
		column_offsets[x] = vftell(result);
		SwapLE32(&column_offsets[x]);
		y = 0;
		while (y < hdr->height) {
			// Scan through to start of next post.
			while (y < hdr->height) {
				alpha = buffer[y * rowstep + x * 4 + 3];
				if (alpha == 0xff) {
					break;
				}
				y++;
			}
			if (y >= hdr->height) {
				break;
			}
			// 0xff indicates end of column, so if the start y has
			// reached this we (1) cannot fit this in one byte;
			// (2) cannot use 0xff as topdelta.
			if (y >= 0xff) {
				vfclose(result);
				goto fail;
			}
			// At this point, we are sure that we are building a
			// post with at least one pixel. We do not allow the
			// post length to reach 0x80, because that is the limit
			// of what vanilla supports for post height.
			post_len = 0;
			post[0] = y;  // topdelta
			while (y < hdr->height && post_len < MAX_POST_LEN) {
				alpha = buffer[y * rowstep + x * 4 + 3];
				if (alpha != 0xff) {
					// end of post
					break;
				}
				post[post_len + 3] =
					palettized[y * hdr->width + x];
				y++;
				post_len++;
			}
			post[1] = post_len; // length
			// Overflow bytes
			post[2] = post[3];
			post[post_len + 3] = post[post_len + 2];
			vfwrite(post, 1, post_len + 4, result);
		}

		// end of column.
		post[0] = 0xff;
		vfwrite(post, 1, 1, result);
	}

	// Now go back and write the real column directory.
	vfseek(result, 8, SEEK_SET);
	vfwrite(column_offsets, sizeof(uint32_t), hdr->width, result);

	// We're done.
	vfseek(result, 0, SEEK_SET);

fail:
	free(column_offsets);
	free(post);
	free(palettized);

	return result;
}

static int UserChunkCallback(png_structp ppng, png_unknown_chunkp chunk)
{
	struct patch_header *hdr = png_get_user_chunk_ptr(ppng);
	struct offsets_chunk offs;

	if (strncmp((const char *) chunk->name, OFFSET_CHUNK_NAME, 4) != 0
	 || chunk->size < sizeof(struct offsets_chunk)) {
		return 1;
	}

	offs = *((const struct offsets_chunk *) chunk->data);
	SwapOffsetsChunk(&offs);

	if (hdr->leftoffset >= INT16_MIN && hdr->leftoffset <= INT16_MAX
	 && hdr->topoffset >= INT16_MIN && hdr->topoffset <= INT16_MAX) {
		hdr->leftoffset = offs.leftoffset;
		hdr->topoffset = offs.topoffset;
	}
	return 1;
}

static uint8_t *ReadPNG(VFILE *input, struct patch_header *hdr,
                        int *rowstep)
{
	struct png_context ctx;
	int bit_depth, color_type, ilace_type, comp_type, filter_method, y;
	png_uint_32 width, height;
	uint8_t *imgbuf = NULL;

	hdr->leftoffset = 0;
	hdr->topoffset = 0;

	if (!V_OpenPNGRead(&ctx, input)) {
		goto fail1;
	}

	png_set_read_user_chunk_fn(ctx.ppng, hdr, UserChunkCallback);
	png_read_info(ctx.ppng, ctx.pinfo);

	png_get_IHDR(ctx.ppng, ctx.pinfo, &width, &height, &bit_depth,
	             &color_type, &ilace_type, &comp_type, &filter_method);

	// Sanity check.
	if (width >= UINT16_MAX || height >= UINT16_MAX) {
		ConversionError("PNG dimensions too large: %d, %d",
		                (int) width, (int) height);
		goto fail2;
	}

	hdr->width = width;
	hdr->height = height;
	// Convert all input files to RGBA format.
	png_set_add_alpha(ctx.ppng, 0xff, PNG_FILLER_AFTER);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(ctx.ppng);
	}
	if (png_get_valid(ctx.ppng, ctx.pinfo, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(ctx.ppng);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(ctx.ppng);
	}
	if (bit_depth < 8) {
		png_set_packing(ctx.ppng);
	}

	png_read_update_info(ctx.ppng, ctx.pinfo);

	*rowstep = png_get_rowbytes(ctx.ppng, ctx.pinfo);
	imgbuf = checked_malloc(*rowstep * height);

	for (y = 0; y < height; ++y) {
		png_read_row(ctx.ppng, imgbuf + y * *rowstep, NULL);
		/* debugging code
		{
			int x;
			printf("%5d: ", y);
			for (x = 0; x < width; ++x) {
				int c = imgbuf[y * rowstep + x * 4];
				printf("%c", c ? '#' : ' ');
			}
			printf("\r\n");
		}*/
	}

	png_read_end(ctx.ppng, NULL);
fail2:
	V_ClosePNG(&ctx);
fail1:
	return imgbuf;
}

static VFILE *BufferToRaw(uint8_t *imgbuf, int rowstep,
                          struct patch_header *hdr)
{
	uint8_t *palettized;
	VFILE *result = vfopenmem(NULL, 0);

	palettized = V_PalettizeRGBABuffer(
		doom_palette, imgbuf, rowstep, hdr->width, hdr->height);
	assert(vfwrite(palettized, hdr->width,
	               hdr->height, result) == hdr->height);
	free(palettized);

	// Rewind so that the caller can read from the stream.
	vfseek(result, 0, SEEK_SET);

	return result;
}

VFILE *V_FromImageFile(VFILE *input)
{
	VFILE *result = NULL;
	struct patch_header hdr;
	uint8_t *imgbuf;
	int rowstep;

	imgbuf = ReadPNG(input, &hdr, &rowstep);
	vfclose(input);
	if (imgbuf == NULL) {
		goto fail;
	}

	result = RGBABufferToPatch(imgbuf, rowstep, &hdr);
	free(imgbuf);
fail:
	return result;
}

VFILE *V_FullscreenFromImageFile(VFILE *input)
{
	struct patch_header hdr;
	VFILE *result = NULL;
	uint8_t *imgbuf;
	int rowstep;

	imgbuf = ReadPNG(input, &hdr, &rowstep);
	if (imgbuf == NULL) {
		goto fail;
	}

	// Do something sensible as a fallback.
	if (hdr.width != FULLSCREEN_W || hdr.height != FULLSCREEN_H) {
		vfseek(input, 0, SEEK_SET);
		return V_FromImageFile(input);
	}

	result = BufferToRaw(imgbuf, rowstep, &hdr);
fail:
	free(imgbuf);
	vfclose(input);
	return result;
}

VFILE *V_FlatFromImageFile(VFILE *input)
{
	VFILE *result = NULL;
	struct patch_header hdr;
	uint8_t *imgbuf = NULL;
	int rowstep;

	imgbuf = ReadPNG(input, &hdr, &rowstep);
	vfclose(input);
	if (imgbuf == NULL) {
		goto fail;
	}
	// Most flats are 64x64. Heretic/Hexen use taller ones, but
	// they are always 64 pixels wide.
	if (hdr.width != 64) {
		result = RGBABufferToPatch(imgbuf, rowstep, &hdr);
		free(imgbuf);
		return result;
	}

	result = BufferToRaw(imgbuf, rowstep, &hdr);
fail:
	free(imgbuf);
	return result;
}

static bool DrawPatch(const struct patch_header *hdr, uint8_t *srcbuf,
                      size_t srcbuf_len, uint8_t *dstbuf)
{
	uint32_t *columnofs =
		(uint32_t *) (srcbuf + sizeof(struct patch_header));
	uint32_t off;
	int x, y, i, cnt;

	memset(dstbuf, TRANSPARENT, hdr->width * hdr->height);

	for (x = 0; x < hdr->width; ++x) {
		off = columnofs[x];
		SwapLE32(&off);
		if (off > srcbuf_len - 1) {
			ConversionError("Corrupted patch: column %d has "
			                "invalid offset %d > %d",
			                x, off, srcbuf_len - 1);
			return false;
		}
		while (srcbuf[off] != 0xff) {
			if (off >= srcbuf_len - 2
			 || off + srcbuf[off + 1] + 4 >= srcbuf_len) {
				ConversionError("Corrupted patch: column %d "
				                "overruns the end of lump", x);
				return false;
			}
			y = srcbuf[off];
			cnt = srcbuf[off + 1];
			off += 3;
			for (i = 0; i < cnt; i++, y++) {
				if (y < hdr->height) {
					dstbuf[y * hdr->width + x] =
						srcbuf[off];
				}
				off++;
			}
			off++;
		}
	}

	return true;
}

static VFILE *WritePNG(struct patch_header *hdr, uint8_t *imgbuf,
                       const png_color *palette)
{
	VFILE *result = NULL;
	uint8_t *alphabuf;
	struct png_context ctx;
	int y;

	result = V_OpenPNGWrite(&ctx);
	if (result == NULL) {
		return NULL;
	}

	alphabuf = checked_malloc(256);
	memset(alphabuf, 0xff, 256);
	alphabuf[TRANSPARENT] = 0;

	png_set_IHDR(ctx.ppng, ctx.pinfo, hdr->width, hdr->height, 8,
	             PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_tRNS(ctx.ppng, ctx.pinfo, alphabuf, 256, NULL);
	png_set_PLTE(ctx.ppng, ctx.pinfo, palette, 256);
	png_write_info(ctx.ppng, ctx.pinfo);
	if (hdr->topoffset != 0 || hdr->leftoffset != 0) {
		WriteOffsetChunk(ctx.ppng, hdr);
	}

	for (y = 0; y < hdr->height; y++) {
		png_write_row(ctx.ppng, imgbuf + y * hdr->width);
	}

	png_write_end(ctx.ppng, ctx.pinfo);
	vfseek(result, 0, SEEK_SET);
	free(alphabuf);

	V_ClosePNG(&ctx);
	return result;
}

VFILE *V_ToImageFile(VFILE *input)
{
	uint8_t *buf, *imgbuf = NULL;
	struct patch_header hdr;
	size_t buf_len;
	VFILE *result = NULL;

	buf = vfreadall(input, &buf_len);
	vfclose(input);
	if (buf_len < 6) {
		ConversionError("Patch too short: %d < 6", (int) buf_len);
		goto fail;
	}

	hdr = *((struct patch_header *) buf);
	V_SwapPatchHeader(&hdr);
	imgbuf = checked_malloc(hdr.width * hdr.height);
	if (!DrawPatch(&hdr, buf, buf_len, imgbuf)) {
		goto fail;
	}

	result = WritePNG(&hdr, imgbuf, doom_palette);

fail:
	free(imgbuf);
	free(buf);

	return result;
}

VFILE *V_FlatToImageFile(VFILE *input)
{
	uint8_t *buf;
	struct patch_header hdr;
	size_t buf_len;
	VFILE *result = NULL;

	buf = vfreadall(input, &buf_len);
	vfclose(input);

	// Most flats are 64x64, but Heretic/Hexen animated ones are larger.
	if (buf_len < 4096 || (buf_len % 64) != 0) {
		ConversionError("Flat lump should be a multiple of 64 bytes.");
		goto fail;
	}

	hdr.width = 64;
	hdr.height = buf_len / 64;
	hdr.topoffset = 0;
	hdr.leftoffset = 0;
	result = WritePNG(&hdr, buf, doom_palette);

fail:
	free(buf);

	return result;
}

// For Hexen fullscreen images.
VFILE *V_FullscreenToImageFile(VFILE *input)
{
	uint8_t *buf;
	struct patch_header hdr;
	size_t buf_len;
	VFILE *result = NULL;

	buf = vfreadall(input, &buf_len);
	vfclose(input);
	assert(buf_len == FULLSCREEN_SZ);

	hdr.width = FULLSCREEN_W;
	hdr.height = FULLSCREEN_H;
	hdr.topoffset = 0;
	hdr.leftoffset = 0;
	result = WritePNG(&hdr, buf, doom_palette);
	free(buf);

	return result;
}

static void ReadHiresPalette(uint8_t *buf, png_color *palette)
{
	unsigned int i, r, g, b;

	for (i = 0; i < 16; i++) {
		r = buf[i * 3];
		g = buf[i * 3 + 1];
		b = buf[i * 3 + 2];
		palette[i].red = (r << 2) | (r >> 4);
		palette[i].green = (g << 2) | (g >> 4);
		palette[i].blue = (b << 2) | (b >> 4);
	}
}

static uint8_t *PlanarToFlat(void *src, png_color *palette)
{
	const uint8_t *srcptrs[4];
	uint8_t srcbits[4], *dest, *result;
	int x, y, i, bit;

	// Set up source pointers to read from source buffer - each 4-bit
	// pixel has its bits split into four sub-buffers
	for (i = 0; i < 4; ++i) {
		srcptrs[i] = src + (i * HIRES_SCREEN_W * HIRES_SCREEN_H / 8);
	}

	// Draw each pixel
	bit = 0;
	result = checked_calloc(HIRES_SCREEN_W, HIRES_SCREEN_H);
	for (y = 0; y < HIRES_SCREEN_H; ++y) {
		dest = result + y * HIRES_SCREEN_W;

		for (x = 0; x < HIRES_SCREEN_W; ++x) {
			// Get the bits for this pixel
			// For each bit, find the byte containing it, shift down
			// and mask out the specific bit wanted.
			for (i = 0; i < 4; ++i) {
				uint8_t b = srcptrs[i][bit / 8];
				srcbits[i] = (b >> (7 - (bit % 8))) & 0x1;
			}

			// Reassemble the pixel value
			*dest = (srcbits[0] << 0) | (srcbits[1] << 1)
			      | (srcbits[2] << 2) | (srcbits[3] << 3);

			// Next pixel!
			++dest;
			++bit;
		}
	}

	return result;
}

VFILE *V_HiresToImageFile(VFILE *input)
{
	VFILE *result;
	uint8_t *lump, *screenbuf;
	size_t lump_len;
	png_color palette[16];
	struct patch_header hdr;

	lump = vfreadall(input, &lump_len);
	vfclose(input);
	if (lump_len < HIRES_MIN_LENGTH) {
		ConversionError("Hires image too short: %d < %d bytes",
		                (int) lump_len, (int) HIRES_MIN_LENGTH);
		free(lump);
		return NULL;
	}

	ReadHiresPalette(lump, palette);
	screenbuf = PlanarToFlat(lump + 3 * 16, palette);

	hdr.width = HIRES_SCREEN_W;
	hdr.height = HIRES_SCREEN_H;
	result = WritePNG(&hdr, screenbuf, palette);

	free(screenbuf);
	free(lump);

	return result;
}
