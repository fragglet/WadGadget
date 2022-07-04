#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui.h"
#include "list_pane.h"

static unsigned int Lines(struct list_pane *p)
{
	int x, y;
	getmaxyx(p->pane, y, x);
	return y - 2;
}

void UI_DrawListPane(struct list_pane *p)
{
	const char *str;
	unsigned int idx, y;

	werase(p->pane);
	wattron(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));
	box(p->pane, 0, 0);
	if (p->active) {
		wattron(p->pane, A_REVERSE);
	}
	mvwaddstr(p->pane, 0, 3, " ");
	waddstr(p->pane, p->title);
	waddstr(p->pane, " ");
	wattroff(p->pane, A_REVERSE);
	wattroff(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));

	for (y = 0; y < Lines(p); y++) {
		char buf[20];
		idx = p->window_offset + y;
		str = p->get_entry_str(p, idx);
		if (str == NULL) {
			continue;
		}
		snprintf(buf, sizeof(buf), "%-20s", str);
		if (p->active && idx == p->selected) {
			wattron(p->pane, A_REVERSE);
		}
		mvwaddstr(p->pane, 1 + y, 1, " ");
		waddstr(p->pane, buf);
		waddstr(p->pane, " ");
		wattroff(p->pane, A_REVERSE);
	}

	wrefresh(p->pane);
}

void UI_ListPaneInput(struct list_pane *p, int key)
{
	unsigned int i;

	switch (key) {
	case KEY_UP:
		if (p->selected > 0) {
			--p->selected;
		}
		if (p->selected < p->window_offset) {
			p->window_offset = p->selected;
		}
		return;
	case KEY_PPAGE:
		for (i = 0; i < Lines(p); i++) {
			UI_ListPaneInput(p, KEY_UP);
		}
		return;
	case KEY_HOME:
		p->selected = 0;
		p->window_offset = 0;
		return;
	case KEY_DOWN:
		if (p->get_entry_str(p, p->selected + 1) != NULL) {
			++p->selected;
		}
		if (p->selected > p->window_offset + Lines(p) - 1) {
			++p->window_offset;
		}
		return;
	case KEY_NPAGE:
		for (i = 0; i < Lines(p); i++) {
			UI_ListPaneInput(p, KEY_DOWN);
		}
		return;
	case KEY_END:
		while (p->get_entry_str(p, p->selected + 1) != NULL) {
			++p->selected;
		}
		p->window_offset = p->selected - Lines(p) + 1;
		return;
	}
}

void UI_ListPaneActive(struct list_pane *p, int active)
{
	p->active = active;
}

