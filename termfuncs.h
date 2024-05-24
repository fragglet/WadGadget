//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>

#define COLORX_DARKGREY       (COLOR_BLACK + 8)
#define COLORX_BRIGHTBLUE     (COLOR_BLUE + 8)
#define COLORX_BRIGHTGREEN    (COLOR_GREEN + 8)
#define COLORX_BRIGHTCYAN     (COLOR_CYAN + 8)
#define COLORX_BRIGHTRED      (COLOR_RED + 8)
#define COLORX_BRIGHTMAGENTA  (COLOR_MAGENTA + 8)
#define COLORX_BRIGHTYELLOW   (COLOR_YELLOW + 8)
#define COLORX_BRIGHTWHITE    (COLOR_WHITE + 8)

struct palette {
	size_t num_colors;
	struct { int c, r, g, b; } colors[16];
};

void TF_SetCursesModes(void);
void TF_SavePalette(struct palette *p);
void TF_SetPalette(struct palette *p);
void TF_SetColorPairs(void);
void TF_SetNewPalette(void);
void TF_RestoreOldPalette(void);

