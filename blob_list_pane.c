#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "colors.h"
#include "common.h"
#include "ui.h"
#include "blob_list_pane.h"

static void SummarizeSize(int64_t len, char buf[10])
{
	if (len < 0) {
		snprintf(buf, 10, "");
	} else if (len < 1000000) {
		snprintf(buf, 10, " %lld", len);
	} else if (len < 1000000000) {
		snprintf(buf, 10, " %lldK", len / 1000);
	} else if (len < 1000000000000) {
		snprintf(buf, 10, " %lldM", len / 1000000);
	} else if (len < 1000000000000000) {
		snprintf(buf, 10, " %lldG", len / 1000000000);
	} else {
		snprintf(buf, 10, " big!");
	}
}

static void DrawEntry(WINDOW *win, int idx, void *data)
{
	struct blob_list_pane *lp = data;
	const struct blob_list_entry *ent;
	static char buf[128];
	unsigned int w, h;
	char size[10] = "";

	getmaxyx(win, h, w);
	w -= 2; h = h;
	if (w > sizeof(buf)) {
		w = sizeof(buf);
	}

	if (idx == 0) {
		snprintf(buf, w, "^ %s",
		         or_if_null(lp->blob_list->parent_dir, ""));
		wattron(win, COLOR_PAIR(PAIR_DIRECTORY));
	} else {
		char prefix = ' ';

		ent = lp->blob_list->get_entry(lp->blob_list, idx - 1);
		if (ent == NULL) {
			return;
		}
		switch (ent->type) {
			case BLOB_TYPE_DIR:
				wattron(win, A_BOLD);
				prefix = '/';
				break;
			case BLOB_TYPE_WAD:
				wattron(win, COLOR_PAIR(PAIR_WAD_FILE));
				break;
			default:
				wattron(win, COLOR_PAIR(PAIR_WHITE_BLACK));
				break;
		}
		SummarizeSize(ent->size, size);
		snprintf(buf, w, "%c%-200s", prefix, ent->name);
	}
	if (lp->pane.active && idx == lp->pane.selected) {
		wattron(win, A_REVERSE);
	} else if (BL_IsTagged(&lp->blob_list->tags, idx - 1)) {
		wattron(win, COLOR_PAIR(PAIR_TAGGED));
	}
	mvwaddstr(win, 0, 0, buf);
	waddstr(win, " ");
	if (idx == 0) {
		mvwaddch(win, 0, 0, ACS_LLCORNER);
		mvwaddch(win, 0, 1, ACS_HLINE);
	}
	mvwaddstr(win, 0, w - strlen(size), size);
	wattroff(win, A_REVERSE);
	wattroff(win, A_BOLD);
	wattroff(win, COLOR_PAIR(PAIR_WHITE_BLACK));
	wattroff(win, COLOR_PAIR(PAIR_DIRECTORY));
	wattroff(win, COLOR_PAIR(PAIR_WAD_FILE));
	wattroff(win, COLOR_PAIR(PAIR_TAGGED));
}

static unsigned int NumEntries(void *data)
{
	struct blob_list_pane *lp = data;
	int result;

	// TODO: This is a terrible hack until a proper num_entries method
	// is added to blob_list.
	result = lp->pane.selected;
	while (lp->blob_list->get_entry(lp->blob_list, result) != NULL) {
		++result;
	}

	return result + 1;
}

void UI_BlobListPaneSearch(void *p, char *needle)
{
	const struct blob_list_entry *ent;
	struct blob_list_pane *lp = p;
	size_t haystack_len, needle_len = strlen(needle);
	int i, j;

	if (strlen(needle) == 0) {
		return;
	}

	if (!strcmp(needle, "..")) {
		lp->pane.selected = 0;
		lp->pane.window_offset = 0;
		return;
	}

	// Check for prefix first, so user can type entire lump name.
	for (i = 0;; i++) {
		ent = lp->blob_list->get_entry(lp->blob_list, i);
		if (ent == NULL) {
			break;
		}
		if (!strncasecmp(ent->name, needle, needle_len)) {
			lp->pane.selected = i + 1;
			lp->pane.window_offset = lp->pane.selected >= 10 ?
			    lp->pane.selected - 10 : 0;
			return;
		}
	}

	// Second time through, we look for a substring match.
	for (i = 0;; i++) {
		ent = lp->blob_list->get_entry(lp->blob_list, i);
		if (ent == NULL) {
			break;
		}
		haystack_len = strlen(ent->name);
		if (haystack_len < needle_len) {
			continue;
		}
		for (j = 0; j < haystack_len - needle_len; j++) {
			if (!strncasecmp(&ent->name[j], needle, needle_len)) {
				lp->pane.selected = i + 1;
				lp->pane.window_offset = lp->pane.selected >= 10 ?
				    lp->pane.selected - 10 : 0;
				return;
			}
		}
	}
}

int UI_BlobListPaneSelected(struct blob_list_pane *p)
{
	return UI_ListPaneSelected(&p->pane) - 1;
}

void UI_BlobListPaneKeypress(void *p, int key)
{
	struct blob_list_pane *lp = p;
	struct blob_tag_list *tags = &lp->blob_list->tags;

	switch (key) {
	case KEY_F(10):
		BL_ClearTags(&lp->blob_list->tags);
		return;
	case ' ':
		if (lp->pane.selected > 0) {
			if (BL_IsTagged(tags, UI_BlobListPaneSelected(lp))) {
				BL_RemoveTag(tags, UI_BlobListPaneSelected(lp));
			} else {
				BL_AddTag(tags, UI_BlobListPaneSelected(lp));
			}
		}
		UI_ListPaneKeypress(lp, KEY_DOWN);
		return;
	default:
		UI_ListPaneKeypress(lp, key);
	}
}

static const struct list_pane_funcs blob_list_pane_funcs = {
	DrawEntry,
	NumEntries,
};

void UI_BlobListPaneInit(struct blob_list_pane *p, WINDOW *w)
{
	UI_ListPaneInit(&p->pane, w, &blob_list_pane_funcs, p);
	p->pane.pane.keypress = UI_BlobListPaneKeypress;
}

const struct blob_list_pane_action *UI_BlobListPaneActions(
	struct blob_list_pane *p, struct blob_list_pane *other)
{
	return p->get_actions(other);
}

enum blob_type UI_BlobListPaneEntryType(struct blob_list_pane *p, unsigned int idx)
{
	const struct blob_list_entry *ent;
	if (idx == 0) {
		return BLOB_TYPE_DIR;
	}
	ent = p->blob_list->get_entry(p->blob_list, idx - 1);
	return ent->type;
}

const char *UI_BlobListPaneEntryPath(struct blob_list_pane *p, unsigned int idx)
{
	if (idx == 0) {
		return p->blob_list->parent_dir;
	}
	return p->blob_list->get_entry_path(p->blob_list, idx - 1);
}

void UI_BlobListPaneFree(struct blob_list_pane *p)
{
	free(p);
}

// TODO: None of the stuff below really belongs in this file.

struct blob_list_pane_action common_actions[] =
{
	{"Space", "Mark/unmark"},
	{"F10", "Unmark all"},
	{"", ""},
	{"Tab", "Other pane"},
	{"ESC", "Quit"},
	{NULL, NULL},
};

static void ShowAction(struct actions_pane *p, int y,
                       const struct blob_list_pane_action *action)
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

