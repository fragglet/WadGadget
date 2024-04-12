//
// Copyright(C) 2022-2024 Simon Howard
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "pane.h"
#include "text_input.h"

#define FILE_PANE_WIDTH  27
#define FILE_PANE_HEIGHT 24

void UI_InitHeaderPane(struct pane *pane, WINDOW *win);

int UI_StringWidth(char *s);
int UI_StringHeight(char *s);
void UI_PrintMultilineString(WINDOW *win, int y, int x, const char *s);

