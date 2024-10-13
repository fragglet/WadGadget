//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI__LIST_PANE_H_INCLUDED
#define UI__LIST_PANE_H_INCLUDED

#include <limits.h>
#include "ui/pane.h"

#define LIST_PANE_END_MARKER  INT_MAX

struct list_pane_funcs {
	void (*draw_element)(WINDOW *w, int index, void *data);
	unsigned int (*num_entries)(void *data);
};

struct list_pane {
	struct pane pane;
	const struct list_pane_funcs *funcs;
	void *data;  // for funcs
	char *title;
	unsigned int window_offset, selected;
	int active;
	unsigned int num_entries;
	WINDOW *subwin;
};

void UI_ListPaneInit(struct list_pane *p, WINDOW *w,
                     const struct list_pane_funcs *funcs, void *data);
bool UI_ListPaneDraw(void *p);
void UI_ListPaneKeypress(void *p, int key);
void UI_ListPaneFree(struct list_pane *p);
int UI_ListPaneSelected(struct list_pane *p);
unsigned int UI_ListPaneLines(struct list_pane *lp);
void UI_ListPaneSetTitle(struct list_pane *lp, const char *title);
void UI_ListPaneSelect(struct list_pane *p, unsigned int idx);

#endif /* #ifndef UI__LIST_PANE_H_INCLUDED */
