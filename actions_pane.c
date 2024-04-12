//
// Copyright(C) 2022-2024 Simon Howard
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <string.h>

#include "actions_pane.h"
#include "colors.h"

static const struct action wad_to_wad[] = {
	{"F2", "Move (rearrange)"},
	{"F3", "View/Edit"},
	{"F4", "> Update"},
	{"F5", "> Copy"},
	{"F6", "Rename"},
	{"F7", "New lump"},
	{"F8", "Delete"},
	{NULL, NULL},
};

static const struct action wad_to_dir[] = {
	{"F2", "Move (rearrange)"},
	{"F3", "View/Edit"},
	{"F5", "> Export"},
	{"F6", "Rename"},
	{"F7", "New lump"},
	{"F8", "Delete"},
	{"F9", "> Export as WAD"},
	{NULL, NULL},
};

static const struct action dir_to_wad[] = {
	{"F3", "View/Edit"},
	{"F4", "> Update"},
	{"F5", "> Import"},
	{"F6", "Rename"},
	{"F7", "Mkdir"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

static const struct action dir_to_dir[] = {
	{"F3", "View/Edit"},
	{"F5", "> Copy"},
	{"F6", "Rename"},
	{"F7", "Mkdir"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

struct action common_actions[] =
{
	{"Space", "Mark/unmark"},
	{"F10", "Unmark all"},
	{"", ""},
	{"Tab", "Other pane"},
	{"^N", "Search again"},
	{"ESC", "Quit"},
	{NULL, NULL},
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
	const struct action *actions;
	WINDOW *win = p->pane.window;
	int i, y;

	actions = action_lists[p->active != FILE_TYPE_DIR]
	                      [p->other != FILE_TYPE_DIR];

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 0, 2, " Actions ");

	y = 1;
	for (i = 0; actions != NULL && actions[i].key != NULL; i++) {
		ShowAction(p, y, &actions[i]);
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
