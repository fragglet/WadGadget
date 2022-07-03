#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui.h"
#include "wad_pane.h"

struct wad_pane {
	WINDOW *pane;
	struct wad_file *f;
	unsigned int window_offset, selected;
};

struct wad_pane *UI_NewWadPane(WINDOW *pane, struct wad_file *f)
{
	struct wad_pane *p;
	p = calloc(1, sizeof(struct wad_pane));
	p->pane = pane;
	p->f = f;
	p->window_offset = 0;
	p->selected = 0;
	return p;
}

void UI_DrawWadPane(struct wad_pane *p)
{
	const struct wad_file_entry *directory;
	unsigned int idx, y;

	werase(p->pane);
	wattron(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));
	box(p->pane, 0, 0);
	wattron(p->pane, A_REVERSE);
	mvwaddstr(p->pane, 0, 3, " foobar.wad ");
	wattroff(p->pane, A_REVERSE);

	directory = W_GetDirectory(p->f);
	for (y = 0; y < 20; y++) {
		char buf[9];
		idx = p->window_offset + y;
		if (idx >= W_NumLumps(p->f)) {
			continue;
		}
		snprintf(buf, sizeof(buf), "%8s", directory[idx].name);
		if (idx == p->selected) {
			wattron(p->pane, A_REVERSE);
		}
		mvwaddstr(p->pane, 1 + idx, 2, buf);
		wattroff(p->pane, A_REVERSE);
	}

	wrefresh(p->pane);
}

