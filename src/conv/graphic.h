//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__GRAPHIC_H_INCLUDED
#define CONV__GRAPHIC_H_INCLUDED

#include "fs/vfile.h"
#include "palette/palette.h"

struct patch_header {
	uint16_t width, height;
	int16_t leftoffset, topoffset;
};

VFILE *V_ToImageFile(VFILE *input, const struct palette *pal);
VFILE *V_FromImageFile(VFILE *input, const struct palette *pal);
VFILE *V_FlatToImageFile(VFILE *input, const struct palette *pal);
VFILE *V_FlatFromImageFile(VFILE *input, const struct palette *pal);
VFILE *V_FullscreenToImageFile(VFILE *input, const struct palette *pal);
VFILE *V_FullscreenFromImageFile(VFILE *input, const struct palette *pal);
VFILE *V_HiresToImageFile(VFILE *input);
void V_SwapPatchHeader(struct patch_header *hdr);

#endif /* #ifndef CONV__GRAPHIC_H_INCLUDED */
