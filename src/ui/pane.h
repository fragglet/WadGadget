//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI__PANE_H_INCLUDED
#define UI__PANE_H_INCLUDED

#include <curses.h>
#include <stdbool.h>

struct pane_stack;

struct pane {
	WINDOW *window;
	bool (*draw)(void *pane);
	void (*keypress)(void *pane, int key);
	void (*mouse_click)(void *pane, int x, int y);
	struct pane *next;
};

void UI_PaneShow(void *pane);
int UI_PaneHide(void *pane);
void UI_DrawAllPanes(void);
void UI_RaisePaneToTop(void *pane);
void UI_PaneKeypress(void *pane, int key);
void UI_PaneMouseClick(void *pane, int x, int y);
void UI_StackKeypress(struct pane_stack *s, int key);
void UI_InputKeypress(int key);
void UI_RunMainLoop(void);
void UI_ExitMainLoop(void);
void UI_Init(void);
bool UI_GetMousePosition(struct pane *if_pane, int *x, int *y);

#endif /* #ifndef UI__PANE_H_INCLUDED */
