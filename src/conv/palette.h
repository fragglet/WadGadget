//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__PALETTE_H_INCLUDED
#define CONV__PALETTE_H_INCLUDED

VFILE *V_PaletteFromImageFile(VFILE *input);
VFILE *V_PaletteToImageFile(VFILE *input);
VFILE *V_ColormapToImageFile(VFILE *input, const struct palette *pal);
VFILE *V_ColormapFromImageFile(VFILE *input, const struct palette *pal);

#endif /* #ifndef CONV__PALETTE_H_INCLUDED */
