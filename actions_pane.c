//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <string.h>
#include <limits.h>

#include "actions_pane.h"
#include "colors.h"
#include "ui.h"

static const struct action wad_to_wad[] = {
	{ 0, "Ent", NULL,     "View/Edit"},
	{ 2, "F2",  "Rearr",  "Move (rearrange)"},
	{ 4, "F4",  "Update", "> Update"},
	{ 5, "F5",  "Copy",   "> Copy"},
	{ 6, "F6",  "Rename", "Rename"},
	{ 7, "F7",  "NewLump", "New lump"},
	{ 8, "F8",  "Delete", "Delete"},
	{ 0, NULL,  NULL,     NULL},
};

static const struct action wad_to_dir[] = {
	{ 0, "Ent", NULL,      "View/Edit"},
	{ 2, "F2",  "Rearr",   "Move (rearrange)"},
	{ 3, "F3",  "ExpWAD",  "> Export as WAD"},
	{ 5, "F5",  "Export",  "> Export to files"},
	{ 6, "F6",  "Rename",  "Rename"},
	{ 7, "F7",  "NewLump", "New lump"},
	{ 8, "F8",  "Delete",  "Delete"},
	{ 0, NULL,  NULL,      NULL},
};

static const struct action dir_to_wad[] = {
	{ 0, "Ent", NULL,     "View/Edit"},
	{ 3, "F3",  "MkWAD",  "Make WAD"},
	{ 4, "F4",  "Update", "> Update"},
	{ 5, "F5",  "Import", "> Import"},
	{ 6, "F6",  "Rename", "Rename"},
	{ 7, "F7",  "Mkdir",  "Mkdir"},
	{ 8, "F8",  "Delete", "Delete"},
	{ 0, NULL,  NULL,     NULL},
};

static const struct action dir_to_dir[] = {
	{ 0, "Ent", NULL,     "View/Edit"},
	{ 3, "F3",  "MkWAD",  "Make WAD"},
	{ 5, "F5",  "Copy",   "> Copy"},
	{ 6, "F6",  "Rename", "Rename"},
	{ 7, "F7",  "Mkdir",  "Mkdir"},
	{ 8, "F8",  "Delete", "Delete"},
	{ 0, NULL,  NULL,     NULL},
};

struct action common_actions[] = {
	{ 0, "Space", NULL,       "Mark/unmark"},
	{ 9, "F9",    "MarkPat",  "Mark pattern"},
	{10, "F10",   "UnmrkAll", "Unmark all"},
	{ 0, "",      NULL,       ""},
	{ 0, "Tab",   NULL,       "Other pane"},
	{ 0, "^N",    NULL,       "Search again"},
	{ 0, "ESC",   NULL,       "Quit"},
	{ 0, NULL,    NULL,       NULL},
};

static const struct action *action_lists[2][2] = {
	{dir_to_dir, dir_to_wad},
	{wad_to_dir, wad_to_wad},
};

static void ShowAction(struct actions_pane *p, int y,
                       const struct action *action)
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
	const struct action *actions, *a;
	WINDOW *win = p->pane.window;
	int i, y, main_cnt;

	actions = action_lists[p->active != FILE_TYPE_DIR]
	                      [p->other != FILE_TYPE_DIR];

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	UI_DrawWindowBox(win);
	mvwaddstr(win, 0, 2, " Actions ");

	main_cnt = INT_MAX;
	for (i = 0, y = 1;; i++, y++) {
		if (i < main_cnt && actions[i].key == NULL) {
			// End of main actions
			main_cnt = i;
		}
		if (i < main_cnt) {
			a = &actions[i];
		} else if (common_actions[i - main_cnt].key == NULL) {
			break;
		} else {
			a = &common_actions[i - main_cnt];
		}
		ShowAction(p, y, a);
	}
}

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsPane;
	pane->pane.keypress = NULL;
	pane->active = FILE_TYPE_DIR;
	pane->other = FILE_TYPE_DIR;
}

void UI_ActionsPaneSet(struct actions_pane *pane, enum file_type active,
                       enum file_type other, int left_to_right)
{
	pane->active = active;
	pane->other = other;
	pane->left_to_right = left_to_right;
}

static void DrawActionsBar(void *pane)
{
	struct actions_bar *p = pane;
	WINDOW *win = p->pane.window;
	const struct action *actions, *a, *cells[10];
	int i, j, y, cnt, main_cnt, num_cells = 0, spacing, columns;
	bool first;

	columns = getmaxx(p->pane.window);

	actions = action_lists[p->active != FILE_TYPE_DIR]
	                      [p->other != FILE_TYPE_DIR];

	main_cnt = INT_MAX;
	memset(cells, 0, sizeof(cells));
	for (i = 0, y = 1;; i++, y++) {
		if (i < main_cnt && actions[i].key == NULL) {
			// End of main actions
			main_cnt = i;
		}
		if (i < main_cnt) {
			a = &actions[i];
		} else if (common_actions[i - main_cnt].key == NULL) {
			break;
		} else {
			a = &common_actions[i - main_cnt];
		}
		if (a->f_key != 0) {
			cells[a->f_key - 1] = a;
			++num_cells;
		}
	}

	spacing = columns - 2;
	for (i = 0, cnt = 0; i < 10; ++i) {
		if (cells[i] != NULL) {
			spacing -= 1 + strlen(cells[i]->shortname);
			cnt++;
		}
	}

	wbkgdset(p->pane.window, COLOR_PAIR(PAIR_WHITE_BLACK));
	werase(p->pane.window);
	mvwaddstr(p->pane.window, 0, 0, "");

	if (spacing < 0) {
		spacing = 0;
	} else {
		int i = (spacing - 1) / (cnt - 1);
		int leftover = spacing - (cnt - 1) * i;
		for (j = 0; j < leftover; j++) {
			waddstr(p->pane.window, " ");
		}
		spacing = i;
	}

	for (i = 0, first = true; i < 10; ++i) {
		char buf[10];
		if (cells[i] == NULL) {
			continue;
		}
		wattron(p->pane.window, COLOR_PAIR(PAIR_WHITE_BLACK));
		for (j = 0; !first && j < spacing; j++) {
			waddstr(p->pane.window, " ");
		}
		wattron(win, A_BOLD);
		snprintf(buf, sizeof(buf), "%d", cells[i]->f_key);
		waddstr(p->pane.window, buf);
		wattroff(win, A_BOLD);
		wattron(p->pane.window, COLOR_PAIR(PAIR_HEADER));
		waddstr(p->pane.window, cells[i]->shortname);
		first = false;
	}
}

void UI_ActionsBarInit(struct actions_bar *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsBar;
	pane->pane.keypress = NULL;
	pane->active = FILE_TYPE_DIR;
	pane->other = FILE_TYPE_DIR;
}

void UI_ActionsBarSet(struct actions_bar *pane, enum file_type active,
                      enum file_type other)
{
	pane->active = active;
	pane->other = other;
}
