//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "fs/vfile.h"

struct patch_header {
	uint16_t width, height;
	int16_t leftoffset, topoffset;
};

VFILE *V_ToImageFile(VFILE *input);
VFILE *V_FromImageFile(VFILE *input);
VFILE *V_FlatFromImageFile(VFILE *input);
VFILE *V_FlatToImageFile(VFILE *input);
void V_SwapPatchHeader(struct patch_header *hdr);
