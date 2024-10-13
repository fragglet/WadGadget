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
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "fs/vfile.h"
#include "palette/palette.h"
#include "conv/error.h"
#include "conv/graphic.h"
#include "conv/vpng.h"

#define PALETTE_SIZE   (256 * 3)

VFILE *V_PaletteFromImageFile(VFILE *input)
{
	struct palette_set *set = PAL_FromImageFile(input);
	VFILE *result;

	if (set == NULL) {
		return NULL;
	}

	result = PAL_MarshalPaletteSet(set);
	PAL_FreePaletteSet(set);
	return result;
}

VFILE *V_PaletteToImageFile(VFILE *input)
{
	struct palette_set *set = PAL_UnmarshalPaletteSet(input);
	VFILE *result;

	if (set == NULL) {
		return NULL;
	}

	result = PAL_ToImageFile(set);
	PAL_FreePaletteSet(set);
	return result;
}

VFILE *V_ColormapToImageFile(VFILE *input, const struct palette *pal)
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
	result = V_WritePalettizedPNG(&hdr, buf, pal, false, 0);

fail:
	free(buf);

	return result;
}

VFILE *V_ColormapFromImageFile(VFILE *input, const struct palette *pal)
{
	VFILE *result = NULL;
	struct patch_header hdr;
	uint8_t *imgbuf = NULL, *palettized;
	int rowstep;

	imgbuf = V_ReadRGBAPNG(input, &hdr, &rowstep);
	vfclose(input);
	if (imgbuf == NULL) {
		goto fail;
	}

	if ((hdr.width * hdr.height) % 256 != 0) {
		ConversionError("Invalid dimensions to make a colormap "
		                "lump; %dx%d = %d pixels, not a multiple "
		                "of 256.", hdr.width, hdr.height,
		                hdr.width * hdr.height);
		goto fail;
	}

	palettized = V_PalettizeRGBABuffer(pal, imgbuf, rowstep,
	                                   hdr.width, hdr.height);
	result = vfopenmem(NULL, 0);
	assert(vfwrite(palettized, hdr.width,
	               hdr.height, result) == hdr.height);
	vfseek(result, 0, SEEK_SET);
	free(palettized);
fail:
	free(imgbuf);
	return result;
}
