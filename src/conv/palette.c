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
#include <png.h>

#include "common.h"
#include "fs/vfile.h"
#include "conv/error.h"
#include "conv/vpng.h"

#define PALETTE_SIZE   (256 * 3)

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
