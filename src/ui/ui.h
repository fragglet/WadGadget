//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "ui/pane.h"
#include "ui/text_input.h"

#define FILE_PANE_WIDTH  27
#define FILE_PANE_HEIGHT 24

void UI_InitHeaderPane(struct pane *pane, WINDOW *win);

int UI_StringWidth(char *s);
int UI_StringHeight(char *s);
void UI_PrintMultilineString(WINDOW *win, int y, int x, const char *s);
void UI_DrawBox(WINDOW *win, int x, int y, int w, int h);
void UI_DrawWindowBox(WINDOW *win);
void UI_ShowNotice(const char *msg, ...);

