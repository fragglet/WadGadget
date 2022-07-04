#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui.h"
#include "list_pane.h"

void UI_DrawListPane(struct list_pane *p)
{
	const char *str;
	unsigned int idx, y;

	werase(p->pane);
	wattron(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));
	box(p->pane, 0, 0);
	wattron(p->pane, A_REVERSE);
	mvwaddstr(p->pane, 0, 3, " ");
	waddstr(p->pane, p->title);
	waddstr(p->pane, " ");
	wattroff(p->pane, A_REVERSE);
	wattroff(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));

	for (y = 0; y < 20; y++) {
		char buf[20];
		idx = p->window_offset + y;
		str = p->get_entry_str(p, idx);
		if (str == NULL) {
			continue;
		}
		snprintf(buf, sizeof(buf), "%-20s", str);
		if (idx == p->selected) {
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
		break;
	case KEY_PPAGE:
		for (i = 0; i < 20; i++) {
			UI_ListPaneInput(p, KEY_UP);
		}
		break;
	case KEY_DOWN:
		if (p->get_entry_str(p, p->selected + 1) != NULL) {
			++p->selected;
		}
		if (p->selected > p->window_offset + 20 - 1) {
			++p->window_offset;
		}
		break;
	case KEY_NPAGE:
		for (i = 0; i < 20; i++) {
			UI_ListPaneInput(p, KEY_DOWN);
		}
		break;
	}
}

