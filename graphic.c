#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>

#include "common.h"
#include "vfile.h"
#include "graphic.h"

#define OFFSET_CHUNK_NAME  "grAb"

#define TRANSPARENT   247
#define MAX_POST_LEN  0x80

struct offsets_chunk {
	int32_t leftoffset, topoffset;
};

// TODO: Get palette from current WADs
static const png_color doom_palette[256] = {
	{0x00, 0x00, 0x00}, {0x1f, 0x17, 0x0b},
	{0x17, 0x0f, 0x07}, {0x4b, 0x4b, 0x4b},
	{0xff, 0xff, 0xff}, {0x1b, 0x1b, 0x1b},
	{0x13, 0x13, 0x13}, {0x0b, 0x0b, 0x0b},
	{0x07, 0x07, 0x07}, {0x2f, 0x37, 0x1f},
	{0x23, 0x2b, 0x0f}, {0x17, 0x1f, 0x07},
	{0x0f, 0x17, 0x00}, {0x4f, 0x3b, 0x2b},
	{0x47, 0x33, 0x23}, {0x3f, 0x2b, 0x1b},
	{0xff, 0xb7, 0xb7}, {0xf7, 0xab, 0xab},
	{0xf3, 0xa3, 0xa3}, {0xeb, 0x97, 0x97},
	{0xe7, 0x8f, 0x8f}, {0xdf, 0x87, 0x87},
	{0xdb, 0x7b, 0x7b}, {0xd3, 0x73, 0x73},
	{0xcb, 0x6b, 0x6b}, {0xc7, 0x63, 0x63},
	{0xbf, 0x5b, 0x5b}, {0xbb, 0x57, 0x57},
	{0xb3, 0x4f, 0x4f}, {0xaf, 0x47, 0x47},
	{0xa7, 0x3f, 0x3f}, {0xa3, 0x3b, 0x3b},
	{0x9b, 0x33, 0x33}, {0x97, 0x2f, 0x2f},
	{0x8f, 0x2b, 0x2b}, {0x8b, 0x23, 0x23},
	{0x83, 0x1f, 0x1f}, {0x7f, 0x1b, 0x1b},
	{0x77, 0x17, 0x17}, {0x73, 0x13, 0x13},
	{0x6b, 0x0f, 0x0f}, {0x67, 0x0b, 0x0b},
	{0x5f, 0x07, 0x07}, {0x5b, 0x07, 0x07},
	{0x53, 0x07, 0x07}, {0x4f, 0x00, 0x00},
	{0x47, 0x00, 0x00}, {0x43, 0x00, 0x00},
	{0xff, 0xeb, 0xdf}, {0xff, 0xe3, 0xd3},
	{0xff, 0xdb, 0xc7}, {0xff, 0xd3, 0xbb},
	{0xff, 0xcf, 0xb3}, {0xff, 0xc7, 0xa7},
	{0xff, 0xbf, 0x9b}, {0xff, 0xbb, 0x93},
	{0xff, 0xb3, 0x83}, {0xf7, 0xab, 0x7b},
	{0xef, 0xa3, 0x73}, {0xe7, 0x9b, 0x6b},
	{0xdf, 0x93, 0x63}, {0xd7, 0x8b, 0x5b},
	{0xcf, 0x83, 0x53}, {0xcb, 0x7f, 0x4f},
	{0xbf, 0x7b, 0x4b}, {0xb3, 0x73, 0x47},
	{0xab, 0x6f, 0x43}, {0xa3, 0x6b, 0x3f},
	{0x9b, 0x63, 0x3b}, {0x8f, 0x5f, 0x37},
	{0x87, 0x57, 0x33}, {0x7f, 0x53, 0x2f},
	{0x77, 0x4f, 0x2b}, {0x6b, 0x47, 0x27},
	{0x5f, 0x43, 0x23}, {0x53, 0x3f, 0x1f},
	{0x4b, 0x37, 0x1b}, {0x3f, 0x2f, 0x17},
	{0x33, 0x2b, 0x13}, {0x2b, 0x23, 0x0f},
	{0xef, 0xef, 0xef}, {0xe7, 0xe7, 0xe7},
	{0xdf, 0xdf, 0xdf}, {0xdb, 0xdb, 0xdb},
	{0xd3, 0xd3, 0xd3}, {0xcb, 0xcb, 0xcb},
	{0xc7, 0xc7, 0xc7}, {0xbf, 0xbf, 0xbf},
	{0xb7, 0xb7, 0xb7}, {0xb3, 0xb3, 0xb3},
	{0xab, 0xab, 0xab}, {0xa7, 0xa7, 0xa7},
	{0x9f, 0x9f, 0x9f}, {0x97, 0x97, 0x97},
	{0x93, 0x93, 0x93}, {0x8b, 0x8b, 0x8b},
	{0x83, 0x83, 0x83}, {0x7f, 0x7f, 0x7f},
	{0x77, 0x77, 0x77}, {0x6f, 0x6f, 0x6f},
	{0x6b, 0x6b, 0x6b}, {0x63, 0x63, 0x63},
	{0x5b, 0x5b, 0x5b}, {0x57, 0x57, 0x57},
	{0x4f, 0x4f, 0x4f}, {0x47, 0x47, 0x47},
	{0x43, 0x43, 0x43}, {0x3b, 0x3b, 0x3b},
	{0x37, 0x37, 0x37}, {0x2f, 0x2f, 0x2f},
	{0x27, 0x27, 0x27}, {0x23, 0x23, 0x23},
	{0x77, 0xff, 0x6f}, {0x6f, 0xef, 0x67},
	{0x67, 0xdf, 0x5f}, {0x5f, 0xcf, 0x57},
	{0x5b, 0xbf, 0x4f}, {0x53, 0xaf, 0x47},
	{0x4b, 0x9f, 0x3f}, {0x43, 0x93, 0x37},
	{0x3f, 0x83, 0x2f}, {0x37, 0x73, 0x2b},
	{0x2f, 0x63, 0x23}, {0x27, 0x53, 0x1b},
	{0x1f, 0x43, 0x17}, {0x17, 0x33, 0x0f},
	{0x13, 0x23, 0x0b}, {0x0b, 0x17, 0x07},
	{0xbf, 0xa7, 0x8f}, {0xb7, 0x9f, 0x87},
	{0xaf, 0x97, 0x7f}, {0xa7, 0x8f, 0x77},
	{0x9f, 0x87, 0x6f}, {0x9b, 0x7f, 0x6b},
	{0x93, 0x7b, 0x63}, {0x8b, 0x73, 0x5b},
	{0x83, 0x6b, 0x57}, {0x7b, 0x63, 0x4f},
	{0x77, 0x5f, 0x4b}, {0x6f, 0x57, 0x43},
	{0x67, 0x53, 0x3f}, {0x5f, 0x4b, 0x37},
	{0x57, 0x43, 0x33}, {0x53, 0x3f, 0x2f},
	{0x9f, 0x83, 0x63}, {0x8f, 0x77, 0x53},
	{0x83, 0x6b, 0x4b}, {0x77, 0x5f, 0x3f},
	{0x67, 0x53, 0x33}, {0x5b, 0x47, 0x2b},
	{0x4f, 0x3b, 0x23}, {0x43, 0x33, 0x1b},
	{0x7b, 0x7f, 0x63}, {0x6f, 0x73, 0x57},
	{0x67, 0x6b, 0x4f}, {0x5b, 0x63, 0x47},
	{0x53, 0x57, 0x3b}, {0x47, 0x4f, 0x33},
	{0x3f, 0x47, 0x2b}, {0x37, 0x3f, 0x27},
	{0xff, 0xff, 0x73}, {0xeb, 0xdb, 0x57},
	{0xd7, 0xbb, 0x43}, {0xc3, 0x9b, 0x2f},
	{0xaf, 0x7b, 0x1f}, {0x9b, 0x5b, 0x13},
	{0x87, 0x43, 0x07}, {0x73, 0x2b, 0x00},
	{0xff, 0xff, 0xff}, {0xff, 0xdb, 0xdb},
	{0xff, 0xbb, 0xbb}, {0xff, 0x9b, 0x9b},
	{0xff, 0x7b, 0x7b}, {0xff, 0x5f, 0x5f},
	{0xff, 0x3f, 0x3f}, {0xff, 0x1f, 0x1f},
	{0xff, 0x00, 0x00}, {0xef, 0x00, 0x00},
	{0xe3, 0x00, 0x00}, {0xd7, 0x00, 0x00},
	{0xcb, 0x00, 0x00}, {0xbf, 0x00, 0x00},
	{0xb3, 0x00, 0x00}, {0xa7, 0x00, 0x00},
	{0x9b, 0x00, 0x00}, {0x8b, 0x00, 0x00},
	{0x7f, 0x00, 0x00}, {0x73, 0x00, 0x00},
	{0x67, 0x00, 0x00}, {0x5b, 0x00, 0x00},
	{0x4f, 0x00, 0x00}, {0x43, 0x00, 0x00},
	{0xe7, 0xe7, 0xff}, {0xc7, 0xc7, 0xff},
	{0xab, 0xab, 0xff}, {0x8f, 0x8f, 0xff},
	{0x73, 0x73, 0xff}, {0x53, 0x53, 0xff},
	{0x37, 0x37, 0xff}, {0x1b, 0x1b, 0xff},
	{0x00, 0x00, 0xff}, {0x00, 0x00, 0xe3},
	{0x00, 0x00, 0xcb}, {0x00, 0x00, 0xb3},
	{0x00, 0x00, 0x9b}, {0x00, 0x00, 0x83},
	{0x00, 0x00, 0x6b}, {0x00, 0x00, 0x53},
	{0xff, 0xff, 0xff}, {0xff, 0xeb, 0xdb},
	{0xff, 0xd7, 0xbb}, {0xff, 0xc7, 0x9b},
	{0xff, 0xb3, 0x7b}, {0xff, 0xa3, 0x5b},
	{0xff, 0x8f, 0x3b}, {0xff, 0x7f, 0x1b},
	{0xf3, 0x73, 0x17}, {0xeb, 0x6f, 0x0f},
	{0xdf, 0x67, 0x0f}, {0xd7, 0x5f, 0x0b},
	{0xcb, 0x57, 0x07}, {0xc3, 0x4f, 0x00},
	{0xb7, 0x47, 0x00}, {0xaf, 0x43, 0x00},
	{0xff, 0xff, 0xff}, {0xff, 0xff, 0xd7},
	{0xff, 0xff, 0xb3}, {0xff, 0xff, 0x8f},
	{0xff, 0xff, 0x6b}, {0xff, 0xff, 0x47},
	{0xff, 0xff, 0x23}, {0xff, 0xff, 0x00},
	{0xa7, 0x3f, 0x00}, {0x9f, 0x37, 0x00},
	{0x93, 0x2f, 0x00}, {0x87, 0x23, 0x00},
	{0x4f, 0x3b, 0x27}, {0x43, 0x2f, 0x1b},
	{0x37, 0x23, 0x13}, {0x2f, 0x1b, 0x0b},
	{0x00, 0x00, 0x53}, {0x00, 0x00, 0x47},
	{0x00, 0x00, 0x3b}, {0x00, 0x00, 0x2f},
	{0x00, 0x00, 0x23}, {0x00, 0x00, 0x17},
	{0x00, 0x00, 0x0b}, {0x00, 0x00, 0x00},
	{0xff, 0x9f, 0x43}, {0xff, 0xe7, 0x4b},
	{0xff, 0x7b, 0xff}, {0xff, 0x00, 0xff},
	{0xcf, 0x00, 0xcf}, {0x9f, 0x00, 0x9b},
	{0x6f, 0x00, 0x6b}, {0xa7, 0x6b, 0x6b},
};

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

static void ErrorCallback(png_structp p, png_const_charp s)
{
	fprintf(stderr, "libpng error: %s\n", s);
}

static void WarningCallback(png_structp p, png_const_charp s)
{
	fprintf(stderr, "libpng warning: %s\n", s);
}

static void PngReadCallback(png_structp ppng, png_bytep buf, size_t len)
{
	VFILE *vf = png_get_io_ptr(ppng);
	int result;

	memset(buf, 0, len);
	result = vfread(buf, 1, len, vf);
	if (result == 0)
	{
		png_error(ppng, "end of file reached");
	}
	else if (result < 0)
	{
		png_error(ppng, "read error");
	}
}

static void PngWriteCallback(png_structp ppng, png_bytep buf, size_t len)
{
	VFILE *vf = png_get_io_ptr(ppng);
	vfwrite(buf, 1, len, vf);
}

static void PngFlushCallback(png_structp ppng)
{
	// no-op
}

static uint8_t FindColor(const png_color *pal, int r, int g, int b)
{
	int diff, best_diff = INT_MAX, i, best_idx = -1;

	for (i = 0; i < 256; i++) {
		diff = (r - pal[i].red) * (r - pal[i].red)
		     + (g - pal[i].green) * (g - pal[i].green)
		     + (b - pal[i].blue) * (b - pal[i].blue);
		if (diff < best_diff) {
			best_idx = i;
			best_diff = diff;
		}
	}

	return best_idx;
}

static VFILE *RGBABufferToPatch(uint8_t *buffer, size_t rowstep,
                                struct patch_header *hdr)
{
	VFILE *result;
	uint32_t *column_offsets;
	uint8_t *post, *pixel;
	int x, y, post_len;

	result = vfopenmem(NULL, 0);

	// Write header.
	vfwrite(hdr, 8, 1, result);

	// Write fake column directory; we'll go back later
	// and overwrite it with the actual data.
	column_offsets = checked_calloc(hdr->width, sizeof(uint32_t));
	vfwrite(column_offsets, sizeof(uint32_t), hdr->width, result);

	post = checked_calloc(MAX_POST_LEN + 4, 1);
	for (x = 0; x < hdr->width; x++) {
		// TODO: swap
		column_offsets[x] = vftell(result);
		y = 0;
		while (y < hdr->height) {
			// Scan through to start of next post.
			while (y < hdr->height) {
				pixel = &buffer[y * rowstep + x * 4];
				if (pixel[3] == 0xff) { // alpha
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
				pixel = &buffer[y * rowstep + x * 4];
				if (pixel[3] != 0xff) { // alpha
					// end of post
					break;
				}
				post[post_len + 3] = FindColor(
					doom_palette, pixel[0], pixel[1], pixel[2]);
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

	return result;
}

static uint8_t *ReadPNG(VFILE *input, struct patch_header *hdr,
                        int *rowstep)
{
	png_structp ppng;
	png_infop pinfo;
	int bit_depth, color_type, ilace_type, comp_type, filter_method, y;
	png_uint_32 width, height;
	uint8_t *imgbuf = NULL;

	ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING,
	                              NULL, NULL, NULL);
	if (!ppng) {
		goto fail1;
	}

	pinfo = png_create_info_struct(ppng);
	if (!pinfo) {
		goto fail2;
	}

	png_set_read_fn(ppng, input, PngReadCallback);
	png_read_info(ppng, pinfo);

	png_get_IHDR(ppng, pinfo, &width, &height, &bit_depth, &color_type,
	             &ilace_type, &comp_type, &filter_method);

	// Sanity check.
	if (width >= UINT16_MAX || height >= UINT16_MAX) {
		goto fail2;
	}

	hdr->width = width;
	hdr->height = height;
	hdr->leftoffset = 0;
	hdr->topoffset = 0;

	// Convert all input files to RGBA format.
	png_set_add_alpha(ppng, 0xff, PNG_FILLER_AFTER);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(ppng);
	}
	if (png_get_valid(ppng, pinfo, PNG_INFO_tRNS)) {
		png_set_tRNS_to_alpha(ppng);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(ppng);
	}
	if (bit_depth < 8) {
		png_set_packing(ppng);
	}

	png_read_update_info(ppng, pinfo);

	*rowstep = png_get_rowbytes(ppng, pinfo);
	imgbuf = checked_malloc(*rowstep * height);

	for (y = 0; y < height; ++y) {
		png_read_row(ppng, imgbuf + y * *rowstep, NULL);
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

	png_read_end(ppng, NULL);
fail2:
	png_destroy_read_struct(&ppng, &pinfo, NULL);
fail1:
	return imgbuf;
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

VFILE *V_FlatFromImageFile(VFILE *input)
{
	VFILE *result = NULL;
	struct patch_header hdr;
	uint8_t *imgbuf = NULL, *rowdata = NULL, *pixel;
	int x, y, rowstep;

	imgbuf = ReadPNG(input, &hdr, &rowstep);
	vfclose(input);
	if (imgbuf == NULL) {
		goto fail;
	}
	// The image has to be a 64x64 graphic. If it isn't, do a normal
	// patch format conversion as above.
	if (hdr.width != 64 || hdr.height != 64) {
		result = RGBABufferToPatch(imgbuf, rowstep, &hdr);
		free(imgbuf);
		return result;
	}

	result = vfopenmem(NULL, 0);

	rowdata = checked_malloc(hdr.width);
	for (y = 0; y < hdr.height; ++y) {
		for (x = 0; x < hdr.width; ++x) {
			pixel = &imgbuf[y * rowstep + x * 4];
			rowdata[x] = FindColor(
				doom_palette, pixel[0], pixel[1], pixel[2]);
		}
		vfwrite(rowdata, 1, hdr.width, result);
	}

	// Rewind so that the caller can read from the stream.
	vfseek(result, 0, SEEK_SET);
fail:
	free(imgbuf);
	free(rowdata);
	return result;
}

static bool DrawPatch(uint8_t *srcbuf, size_t srcbuf_len, uint8_t *dstbuf)
{
	struct patch_header *hdr = (struct patch_header *) srcbuf;
	uint32_t *columnofs =
		(uint32_t *) (srcbuf + sizeof(struct patch_header));
	int x, y, off, i, cnt;

	memset(dstbuf, TRANSPARENT, hdr->width * hdr->height);

	for (x = 0; x < hdr->width; ++x)
	{
		off = columnofs[x];
		if (off > srcbuf_len - 1)
		{
			return false;
		}
		while (srcbuf[off] != 0xff)
		{
			if (off >= srcbuf_len - 2
			 || off + srcbuf[off + 1] + 4 >= srcbuf_len)
			{
				return false;
			}
			y = srcbuf[off];
			cnt = srcbuf[off + 1];
			off += 3;
			for (i = 0; i < cnt; i++, y++)
			{
				if (y < hdr->height)
				{
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

static VFILE *WritePNG(struct patch_header *hdr, uint8_t *imgbuf)
{
	VFILE *result = NULL;
	uint8_t *alphabuf;
	png_structp ppng;
	png_infop pinfo = NULL;
	int y;

	ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	                               ErrorCallback, WarningCallback);
	if (!ppng)
	{
		return NULL;
	}

	pinfo = png_create_info_struct(ppng);
	if (!pinfo)
	{
		goto fail;
	}

	alphabuf = checked_malloc(256);
	memset(alphabuf, 0xff, 256);
	alphabuf[TRANSPARENT] = 0;

	result = vfopenmem(NULL, 0);
	png_set_write_fn(ppng, result, PngWriteCallback, PngFlushCallback);
	png_set_IHDR(ppng, pinfo, hdr->width, hdr->height, 8,
	             PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
	png_set_tRNS(ppng, pinfo, alphabuf, 256, NULL);
	png_set_PLTE(ppng, pinfo, doom_palette, 256);
	png_write_info(ppng, pinfo);
	if (hdr->topoffset != 0 || hdr->leftoffset != 0) {
		WriteOffsetChunk(ppng, hdr);
	}

	for (y = 0; y < hdr->height; y++)
	{
		png_write_row(ppng, imgbuf + y * hdr->width);
	}

	png_write_end(ppng, pinfo);
	vfseek(result, 0, SEEK_SET);
	free(alphabuf);

fail:
	png_destroy_write_struct(&ppng, &pinfo);
	return result;
}

VFILE *V_ToImageFile(VFILE *input)
{
	uint8_t *buf, *imgbuf = NULL;
	struct patch_header *hdr;
	size_t buf_len;
	VFILE *bufreader, *result = NULL;

	bufreader = vfopenmem(NULL, 0);
	vfcopy(input, bufreader);
	vfclose(input);

	if (!vfgetbuf(bufreader, (void **) &buf, &buf_len)
	 || buf_len < 6)
	{
		goto fail;
	}

	hdr = (struct patch_header *) buf;
	imgbuf = checked_malloc(hdr->width * hdr->height);
	if (!DrawPatch(buf, buf_len, imgbuf))
	{
		goto fail;
	}

	result = WritePNG(hdr, imgbuf);

fail:
	free(imgbuf);
	vfclose(bufreader);

	return result;
}

VFILE *V_FlatToImageFile(VFILE *input)
{
	uint8_t *buf;
	struct patch_header hdr;
	size_t buf_len;
	VFILE *bufreader, *result = NULL;

	bufreader = vfopenmem(NULL, 0);
	vfcopy(input, bufreader);
	vfclose(input);

	// Lump must be exactly 4096 bytes.
	if (!vfgetbuf(bufreader, (void **) &buf, &buf_len)
	 || buf_len != 4096)
	{
		goto fail;
	}

	hdr.width = 64;
	hdr.height = 64;
	hdr.topoffset = 0;
	hdr.leftoffset = 0;
	result = WritePNG(&hdr, buf);

fail:
	vfclose(bufreader);

	return result;
}
