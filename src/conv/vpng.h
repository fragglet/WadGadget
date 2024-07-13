//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <png.h>

#define PALETTE_TRANSPARENT   247

struct png_context {
	png_structp ppng;
	png_infop pinfo;
	bool write;
};

extern const struct palette doom_palette;

uint8_t *V_PalettizeRGBABuffer(const struct palette *palette, uint8_t *buf,
                               size_t rowstep, int width, int height);

bool V_OpenPNGRead(struct png_context *ctx, VFILE *input);
VFILE *V_OpenPNGWrite(struct png_context *ctx);
void V_ClosePNG(struct png_context *ctx);

uint8_t *V_ReadRGBAPNG(VFILE *input, struct patch_header *hdr, int *rowstep);
VFILE *V_WritePalettizedPNG(struct patch_header *hdr, uint8_t *imgbuf,
                            const struct palette *palette,
                            bool set_transparency);
