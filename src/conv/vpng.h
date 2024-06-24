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

struct png_context {
	png_structp ppng;
	png_infop pinfo;
	bool write;
};

bool V_OpenPNGRead(struct png_context *ctx, VFILE *input);
VFILE *V_OpenPNGWrite(struct png_context *ctx);
void V_ClosePNG(struct png_context *ctx);

