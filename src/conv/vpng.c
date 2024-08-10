//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// Basic libpng wrapper functions for reading and writing PNG files
// through the VFILE interface.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "fs/vfile.h"
#include "palette/palette.h"
#include "conv/error.h"
#include "conv/graphic.h"
#include "conv/vpng.h"

#define OFFSET_CHUNK_NAME  "grAb"

struct offsets_chunk {
	int32_t leftoffset, topoffset;
};

static jmp_buf libpng_abort_jump;

static void SwapOffsetsChunk(struct offsets_chunk *chunk)
{
	SwapBE32(&chunk->leftoffset);
	SwapBE32(&chunk->topoffset);
}

static uint8_t FindColor(const struct palette *pal, int r, int g, int b)
{
	int diff, best_diff = INT_MAX, i, best_idx = -1;

	for (i = 0; i < 256; i++) {
		diff = (r - pal->entries[i].r) * (r - pal->entries[i].r)
		     + (g - pal->entries[i].g) * (g - pal->entries[i].g)
		     + (b - pal->entries[i].b) * (b - pal->entries[i].b);
		if (diff == 0) {
			return i;
		}
		if (diff < best_diff) {
			best_idx = i;
			best_diff = diff;
		}
	}

	return best_idx;
}

uint8_t *V_PalettizeRGBABuffer(const struct palette *pal, uint8_t *buf,
                               size_t rowstep, int width, int height)
{
	uint8_t *result, *pixel;
	int x, y;

	result = checked_calloc(width, height);

	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			pixel = &buf[y * rowstep + x * 4];
			result[y * width + x] = FindColor(
				pal, pixel[0], pixel[1], pixel[2]);
		}
	}

	return result;
}

static void ErrorCallback(png_structp p, png_const_charp s)
{
	ConversionError("%s", s);
	longjmp(libpng_abort_jump, 1);
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
	if (result == 0) {
		png_error(ppng, "end of file reached");
	} else if (result < 0) {
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

void V_ClosePNG(struct png_context *ctx)
{
	if (ctx->write) {
		png_destroy_write_struct(&ctx->ppng, &ctx->pinfo);
	} else {
		png_destroy_read_struct(&ctx->ppng, &ctx->pinfo, NULL);
	}
}

bool V_OpenPNGRead(struct png_context *ctx, VFILE *input)
{
	ctx->ppng = NULL;
	ctx->pinfo = NULL;
	ctx->write = false;

	if (setjmp(libpng_abort_jump) != 0) {
		ConversionError("Error when parsing PNG file");
		V_ClosePNG(ctx);
		return NULL;
	}

	ctx->ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                   ErrorCallback, WarningCallback);
	if (ctx->ppng == NULL) {
		ConversionError("Failed to open PNG file");
		return false;
	}

	ctx->pinfo = png_create_info_struct(ctx->ppng);
	if (ctx->pinfo == NULL) {
		ConversionError("Failed to create PNG info struct");
		V_ClosePNG(ctx);
		return false;
	}

	png_set_read_fn(ctx->ppng, input, PngReadCallback);
	return true;
}

VFILE *V_OpenPNGWrite(struct png_context *ctx)
{
	VFILE *result;

	ctx->ppng = NULL;
	ctx->pinfo = NULL;
	ctx->write = true;

	if (setjmp(libpng_abort_jump) != 0) {
		ConversionError("Error when writing PNG file");
		V_ClosePNG(ctx);
		return NULL;
	}

	ctx->ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                    ErrorCallback, WarningCallback);
	if (!ctx->ppng) {
		ConversionError("Failed to create PNG write struct");
		return NULL;
	}

	ctx->pinfo = png_create_info_struct(ctx->ppng);
	if (!ctx->pinfo) {
		ConversionError("Failed to create PNG info struct");
		V_ClosePNG(ctx);
		return NULL;
	}

	result = vfopenmem(NULL, 0);
	png_set_write_fn(ctx->ppng, result, PngWriteCallback, PngFlushCallback);

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

uint8_t *V_ReadRGBAPNG(VFILE *input, struct patch_header *hdr, int *rowstep)
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

static png_color *MakePNGPalette(const struct palette *palette)
{
	png_color *result = checked_calloc(256, sizeof(png_color));
	int i;

	for (i = 0; i < 256; i++) {
		result[i].red = palette->entries[i].r;
		result[i].green = palette->entries[i].g;
		result[i].blue = palette->entries[i].b;
	}

	return result;
}

VFILE *V_WritePalettizedPNG(struct patch_header *hdr, uint8_t *imgbuf,
                            const struct palette *palette,
                            bool set_transparency, int transparent_color)
{
	png_color *png_pal = MakePNGPalette(palette);
	VFILE *result = NULL;
	uint8_t *alphabuf = NULL;
	struct png_context ctx;
	int y;

	result = V_OpenPNGWrite(&ctx);
	if (result == NULL) {
		free(png_pal);
		return NULL;
	}

	png_set_IHDR(ctx.ppng, ctx.pinfo, hdr->width, hdr->height, 8,
	             PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
	             PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	if (set_transparency) {
		alphabuf = checked_malloc(256);
		memset(alphabuf, 0xff, 256);
		alphabuf[transparent_color] = 0;

		png_set_tRNS(ctx.ppng, ctx.pinfo, alphabuf, 256, NULL);
	}
	png_set_PLTE(ctx.ppng, ctx.pinfo, png_pal, 256);
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
	free(png_pal);
	return result;
}
