//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui/colors.h"
#include "common.h"
#include "ui/ui.h"
#include "ui/list_pane.h"

static unsigned int NumEntries(struct list_pane *lp)
{
	return lp->funcs->num_entries(lp->data);
}

unsigned int UI_ListPaneLines(struct list_pane *lp)
{
	return getmaxy(lp->pane.window) - 2;
}

void UI_ListPaneDraw(void *p)
{
	struct list_pane *lp = p;
	WINDOW *win = lp->pane.window;
	unsigned int y, idx, num_entries;
	int wx;

	wx = getmaxx(lp->pane.window);

	if (lp->subwin == NULL) {
		lp->subwin = derwin(win, 1, wx - 1, 0, 0);
	}
	wresize(lp->subwin, 1, wx - 1);

	werase(win);
	wattron(win, COLOR_PAIR(PAIR_PANE_COLOR));
	UI_DrawWindowBox(win);
	if (lp->title != NULL) {
		wattron(lp->subwin, COLOR_PAIR(PAIR_PANE_COLOR));
		mvderwin(lp->subwin, 0, 0);
		if (lp->active) {
			wattron(lp->subwin, A_REVERSE);
		}
		mvwaddstr(lp->subwin, 0, 3, " ");
		waddstr(lp->subwin, lp->title);
		waddstr(lp->subwin, " ");
		wattroff(lp->subwin, A_REVERSE);
		wattroff(lp->subwin, COLOR_PAIR(PAIR_PANE_COLOR));
	}
	wattroff(win, COLOR_PAIR(PAIR_PANE_COLOR));

	wresize(lp->subwin, 1, wx - 1);

	// Each list element gets drawn inside a subwindow of the main
	// window. We move the subwindow line by line before drawing.
	num_entries = NumEntries(lp);
	for (y = 0; y < UI_ListPaneLines(lp); y++) {
		idx = lp->window_offset + y;
		if (idx >= num_entries) {
			break;
		}
		mvderwin(lp->subwin, y + 1, 1);
		lp->funcs->draw_element(lp->subwin, idx, lp->data);
	}
	// If we're at the end of the list, we let the list implementation
	// display an end marker.
	if (y < UI_ListPaneLines(lp)) {
		mvderwin(lp->subwin, y + 1, 1);
		lp->funcs->draw_element(lp->subwin, LIST_PANE_END_MARKER,
		                        lp->data);
	}
}

void UI_ListPaneSelect(struct list_pane *p, unsigned int idx)
{
	p->selected = idx;
	if (idx >= p->window_offset &&
	    idx < p->window_offset + UI_ListPaneLines(p)) {
		// Current window offset is okay
		return;
	}
	if (idx < 5) {
		p->window_offset = 0;
	} else {
		p->window_offset = idx - 5;
	}
}

void UI_ListPaneKeypress(void *p, int key)
{
	struct list_pane *lp = p;
	unsigned int i, lines;

	switch (key) {
	case KEY_UP:
		if (lp->selected > 0) {
			--lp->selected;
		}
		if (lp->selected < lp->window_offset) {
			lp->window_offset = lp->selected;
		}
		return;
	case KEY_PPAGE:
		for (i = 0; i < UI_ListPaneLines(lp); i++) {
			UI_ListPaneKeypress(p, KEY_UP);
		}
		return;
	case KEY_HOME:
		lp->selected = 0;
		lp->window_offset = 0;
		return;
	case KEY_DOWN:
		if (lp->selected + 1 < NumEntries(lp)) {
			++lp->selected;
		}
		if (lp->selected > lp->window_offset + UI_ListPaneLines(lp) - 1) {
			++lp->window_offset;
		}
		return;
	case KEY_NPAGE:
		for (i = 0; i < UI_ListPaneLines(lp); i++) {
			UI_ListPaneKeypress(p, KEY_DOWN);
		}
		return;
	case KEY_END:
		lp->selected = NumEntries(lp) - 1;
		lines = UI_ListPaneLines(lp);
		if (lines < lp->selected) {
			lp->window_offset = lp->selected - lines + 1;
		} else {
			lp->window_offset = 0;
		}
		return;
	}
}

void UI_ListPaneInit(struct list_pane *p, WINDOW *w,
                     const struct list_pane_funcs *funcs, void *data)
{
	p->pane.window = w;
	p->pane.draw = UI_ListPaneDraw;
	p->pane.keypress = UI_ListPaneKeypress;
	p->subwin = NULL;
	p->funcs = funcs;
	p->data = data;
}

int UI_ListPaneSelected(struct list_pane *p)
{
	return p->selected;
}

void UI_ListPaneSetTitle(struct list_pane *lp, const char *title)
{
	lp->title = strdup(title);
}

void UI_ListPaneFree(struct list_pane *lp)
{
	free(lp->title);
}
