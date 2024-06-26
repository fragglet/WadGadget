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

#define PALETTE_SIZE   (256 * 3)

VFILE *V_PaletteFromImageFile(VFILE *input)
{
	struct png_context ctx;
	int bit_depth, color_type, ilace_type, comp_type, filter_method;
	int rowstep, y;
	png_uint_32 width, height;
	uint8_t *rowbuf;
	VFILE *result = NULL;

	if (!V_OpenPNGRead(&ctx, input)) {
		return NULL;
	}

	png_read_info(ctx.ppng, ctx.pinfo);
	png_get_IHDR(ctx.ppng, ctx.pinfo, &width, &height, &bit_depth,
	             &color_type, &ilace_type, &comp_type, &filter_method);

	// check dimensions
	if ((width * height) % 256 != 0) {
		ConversionError("Invalid dimensions for palette: %dx%d = "
		                "%d pixels; should be a multiple of 256",
		                width, height, width * height);
		goto fail;
	}

	// Convert all input files to RGB format.
	png_set_strip_alpha(ctx.ppng);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(ctx.ppng);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(ctx.ppng);
	}
	if (bit_depth < 8) {
		png_set_packing(ctx.ppng);
	}

	png_read_update_info(ctx.ppng, ctx.pinfo);

	rowstep = png_get_rowbytes(ctx.ppng, ctx.pinfo);
	rowbuf = checked_malloc(rowstep);
	result = vfopenmem(NULL, 0);

	for (y = 0; y < height; ++y) {
		png_read_row(ctx.ppng, rowbuf, NULL);
		assert(vfwrite(rowbuf, 3, width, result) == width);
	}

	free(rowbuf);
	png_read_end(ctx.ppng, NULL);

	// Rewind so that the caller can read from the stream.
	vfseek(result, 0, SEEK_SET);

fail:
	V_ClosePNG(&ctx);
	vfclose(input);
	return result;
}

VFILE *V_PaletteToImageFile(VFILE *input)
{
	VFILE *result = NULL;
	struct png_context ctx;
	uint8_t *lump = NULL;
	size_t lump_len;
	int npalettes, w, h, y;

	lump = vfreadall(input, &lump_len);
	vfclose(input);

	if ((lump_len % PALETTE_SIZE) != 0) {
		ConversionError("Palette lump has wrong length; should be a "
		                "multiple of %d bytes", PALETTE_SIZE);
		goto fail;
	}

	npalettes = lump_len / PALETTE_SIZE;
	w = 256;
	h = npalettes;

	result = V_OpenPNGWrite(&ctx);
	if (result == NULL) {
		goto fail;
	}

	png_set_IHDR(ctx.ppng, ctx.pinfo, w, h, 8, PNG_COLOR_TYPE_RGB,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);
	png_write_info(ctx.ppng, ctx.pinfo);

	for (y = 0; y < h; y++) {
		png_write_row(ctx.ppng, lump + PALETTE_SIZE * y);
	}

	png_write_end(ctx.ppng, ctx.pinfo);
	vfseek(result, 0, SEEK_SET);

fail:
	V_ClosePNG(&ctx);
	free(lump);
	return result;
}

VFILE *V_ColormapToImageFile(VFILE *input)
{
	uint8_t *buf;
	struct patch_header hdr;
	size_t buf_len;
	VFILE *result = NULL;

	buf = vfreadall(input, &buf_len);
	vfclose(input);

	if (buf_len % 256 != 0) {
		ConversionError("Invalid colormap length: %d is not a "
		                "multiple of 256", buf_len);
		goto fail;
	}

	hdr.width = 256;
	hdr.height = buf_len / 256;
	hdr.topoffset = 0;
	hdr.leftoffset = 0;
	result = V_WritePalettizedPNG(&hdr, buf, doom_palette, false);

fail:
	free(buf);

	return result;
}
