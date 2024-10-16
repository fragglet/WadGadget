//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef SIXEL_DISPLAY_H_INCLUDED
#define SIXEL_DISPLAY_H_INCLUDED

#include <stdbool.h>

bool SIXEL_CheckSupported(void);
void SIXEL_ClearAndPrint(const char *msg, ...);
bool SIXEL_DisplayImage(const char *filename);

#endif /* #ifndef SIXEL_DISPLAY_H_INCLUDED */
