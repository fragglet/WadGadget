#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "ui.h"
#include "list_pane.h"

static unsigned int Lines(struct list_pane *lp)
{
	int x, y;
	getmaxyx(lp->pane.window, y, x);
	x = x;
	return y - 2;
}

static void DrawEntry(struct list_pane *lp, unsigned int idx,
                      unsigned int y)
{
	WINDOW *win = lp->pane.window;
	const char *str;
	static char buf[128];
	unsigned int w, h;

	getmaxyx(win, h, w);
	w -= 2; h = h;
	if (w > sizeof(buf)) {
		w = sizeof(buf);
	}

	if (idx == 0) {
		snprintf(buf, w, "^- %s",
		         or_if_null(lp->blob_list->parent_dir, ""));
		wattron(win, COLOR_PAIR(PAIR_DIRECTORY));
	} else {
		str = lp->blob_list->get_entry_str(lp->blob_list, idx - 1);
		if (str == NULL) {
			return;
		}
		switch (UI_ListPaneEntryType(lp, idx)) {
			case BLOB_TYPE_DIR:
				wattron(win, COLOR_PAIR(PAIR_DIRECTORY));
				wattron(win, A_BOLD);
				break;
			case BLOB_TYPE_WAD:
				wattron(win, COLOR_PAIR(PAIR_WAD_FILE));
				break;
			default:
				break;
		}
		snprintf(buf, w, " %-200s", str);
	}
	if (lp->pane.active && idx == lp->selected) {
		wattron(win, A_REVERSE);
	}
	mvwaddstr(win, 1 + y, 1, buf);
	waddstr(win, " ");
	wattroff(win, A_REVERSE);
	wattroff(win, A_BOLD);
	wattroff(win, COLOR_PAIR(PAIR_DIRECTORY));
	wattroff(win, COLOR_PAIR(PAIR_WAD_FILE));
}

static void Draw(void *p)
{
	struct list_pane *lp = p;
	WINDOW *win = lp->pane.window;
	unsigned int y;

	werase(win);
	wattron(win, COLOR_PAIR(PAIR_PANE_COLOR));
	box(win, 0, 0);
	if (lp->pane.active) {
		wattron(win, A_REVERSE);
	}
	mvwaddstr(win, 0, 3, " ");
	waddstr(win, lp->blob_list->name);
	waddstr(win, " ");
	wattroff(win, A_REVERSE);
	wattroff(win, COLOR_PAIR(PAIR_PANE_COLOR));

	for (y = 0; y < Lines(lp); y++) {
		DrawEntry(lp, lp->window_offset + y, y);
	}
}

static void Keypress(void *p, int key)
{
	struct list_pane *lp = p;
	unsigned int i;

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
		for (i = 0; i < Lines(lp); i++) {
			Keypress(p, KEY_UP);
		}
		return;
	case KEY_HOME:
		lp->selected = 0;
		lp->window_offset = 0;
		return;
	case KEY_DOWN:
		if (lp->blob_list->get_entry_str(
			lp->blob_list, lp->selected + 1 - 1) != NULL) {
			++lp->selected;
		}
		if (lp->selected > lp->window_offset + Lines(lp) - 1) {
			++lp->window_offset;
		}
		return;
	case KEY_NPAGE:
		for (i = 0; i < Lines(lp); i++) {
			Keypress(p, KEY_DOWN);
		}
		return;
	case KEY_END:
		while (lp->blob_list->get_entry_str(
				lp->blob_list, lp->selected + 1 - 1) != NULL) {
			++lp->selected;
		}
		lp->window_offset = lp->selected - Lines(lp) + 1;
		return;
	}
}

void UI_ListPaneInit(struct list_pane *p, WINDOW *w)
{
	p->pane.window = w;
	p->pane.draw = Draw;
	p->pane.keypress = Keypress;
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

struct list_pane_action common_actions[] =
{
	{"Space", "Mark/unmark"},
	{"F10", "Unmark all"},
	{"", ""},
	{"Tab", "Other pane"},
	{"ESC", "Quit"},
	{NULL, NULL},
};

static void ShowAction(struct actions_pane *p, int y,
                       const struct list_pane_action *action)
{
	WINDOW *win = p->pane.window;
	char *desc;

	if (strlen(action->key) == 0) {
		return;
	}
	wattron(win, A_BOLD);
	mvwaddstr(win, y, 2, action->key);
	wattroff(win, A_BOLD);
	waddstr(win, " - ");
	desc = action->description;
	if (action->description[0] == '>') {
		if (!p->left_to_right) {
			wattron(win, A_BOLD);
			waddstr(win, "<<< ");
			wattroff(win, A_BOLD);
		}
		desc += 2;
	}
	waddstr(win, desc);
	if (action->description[0] == '>') {
		if (p->left_to_right) {
			wattron(win, A_BOLD);
			waddstr(win, " >>>");
			wattroff(win, A_BOLD);
		}
	}
}

static void DrawActionsPane(void *pane)
{
	struct actions_pane *p = pane;
	WINDOW *win = p->pane.window;
	int i, y;

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 0, 2, " Actions ");

	y = 1;
	for (i = 0; p->actions != NULL
	         && p->actions[i].key != NULL; i++) {
		ShowAction(p, y, &p->actions[i]);
		y++;
	}
	for (i = 0; common_actions[i].key != NULL; i++) {
		ShowAction(p, y, &common_actions[i]);
		y++;
	}
}

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsPane;
	pane->pane.keypress = NULL;
	pane->actions = NULL;
}

