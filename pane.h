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

#ifndef PANE_H_INCLUDED
#define PANE_H_INCLUDED

#include <curses.h>

struct pane {
	WINDOW *window;
	void (*draw)(void *pane);
	void (*keypress)(void *pane, int key);
};

void UI_PaneKeypress(void *pane, int key);
void UI_PaneShow(void *pane);
int UI_PaneHide(void *pane);
void UI_DrawAllPanes(void);
void UI_RaisePaneToTop(void *pane);
void UI_RunMainLoop(void);
void UI_ExitMainLoop(void);

#endif /* #ifndef PANE_H_INCLUDED */

