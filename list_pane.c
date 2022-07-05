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
	x = x;
	return y - 2;
}

static void DrawEntry(struct list_pane *p, unsigned int idx,
                      unsigned int y)
{
	const char *str;
	static char buf[128];
	unsigned int w, h;

	getmaxyx(p->pane, h, w);
	w -= 2; h = h;
	if (w > sizeof(buf)) {
		w = sizeof(buf);
	}

	if (idx == 0) {
		snprintf(buf, w, "^- %s",
		         p->blob_list->parent_dir != NULL ?
		             p->blob_list->parent_dir : "");
		wattron(p->pane, COLOR_PAIR(PAIR_SPECIAL));
	} else {
		str = p->blob_list->get_entry_str(p->blob_list, idx - 1);
		if (str == NULL) {
			return;
		}
		snprintf(buf, w, " %-200s", str);
	}
	if (p->active && idx == p->selected) {
		wattron(p->pane, A_REVERSE);
	}
	mvwaddstr(p->pane, 1 + y, 1, buf);
	waddstr(p->pane, " ");
	wattroff(p->pane, A_REVERSE);
	wattroff(p->pane, COLOR_PAIR(PAIR_SPECIAL));
}

void UI_DrawListPane(struct list_pane *p)
{
	unsigned int y;

	werase(p->pane);
	wattron(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));
	box(p->pane, 0, 0);
	if (p->active) {
		wattron(p->pane, A_REVERSE);
	}
	mvwaddstr(p->pane, 0, 3, " ");
	waddstr(p->pane, p->blob_list->name);
	waddstr(p->pane, " ");
	wattroff(p->pane, A_REVERSE);
	wattroff(p->pane, COLOR_PAIR(PAIR_PANE_COLOR));

	for (y = 0; y < Lines(p); y++) {
		DrawEntry(p, p->window_offset + y, y);
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
		if (p->blob_list->get_entry_str(
			p->blob_list, p->selected + 1 - 1) != NULL) {
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
		while (p->blob_list->get_entry_str(
				p->blob_list, p->selected + 1 - 1) != NULL) {
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

const struct list_pane_action *UI_ListPaneActions(
	struct list_pane *p, struct list_pane *other)
{
	return p->get_actions(other);
}

enum blob_type UI_ListPaneEntryType(struct list_pane *p, unsigned int idx)
{
	if (idx == 0) {
		return BLOB_TYPE_DIR;
	}
	return p->blob_list->get_entry_type(p->blob_list, idx - 1);
}

const char *UI_ListPaneEntryPath(struct list_pane *p, unsigned int idx)
{
	if (idx == 0) {
		return p->blob_list->parent_dir;
	}
	return p->blob_list->get_entry_path(p->blob_list, idx - 1);
}

void UI_ListPaneFree(struct list_pane *p)
{
	free(p);
}

