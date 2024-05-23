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
#include "common.h"
#include "strings.h"
#include "ui.h"

#define MAX_KEY_BINDINGS 20

static const int key_ordering[] = {
	KEY_ENTER, KEY_F(1), KEY_F(2), KEY_F(3), KEY_F(4), KEY_F(5), KEY_F(6),
	KEY_F(7), KEY_F(8), ' ', KEY_F(9), KEY_F(10), 0, '\t', 'N' & 0x1f,
	0x1f,
};

static const struct action wad_actions[] = {
	{ KEY_F(2),  "Rearr",   "Move (rearrange)"},
	{ KEY_F(7),  "NewLump", "New lump"},
	{ 0 },
};

static const struct action dir_actions[] = {
	{ KEY_F(3),  "MkWAD",  "Make WAD"},
	{ KEY_F(6),  "Ren",    "Rename"},
	{ KEY_F(7),  "Mkdir",  "Mkdir"},
	{ KEY_F(8),  "Del",    "Delete"},
	{ 0 },
};

static const struct action wad_to_wad[] = {
	{ KEY_F(4),  "Upd",     "> Update"},
	{ KEY_F(5),  "Copy",    "> Copy"},
	{ 0 },
};

static const struct action wad_to_dir[] = {
	{ KEY_F(3),  "ExpWAD",  "> Export as WAD"},
	{ KEY_F(5),  "Export",  "> Export to files"},
	{ 0 },
};

static const struct action dir_to_wad[] = {
	{ KEY_F(4),  "Upd",    "> Update"},
	{ KEY_F(5),  "Import", "> Import"},
	{ 0 },
};

static const struct action dir_to_dir[] = {
	{ KEY_F(5),  "Copy",   "> Copy"},
	{ 0 },
};

struct action common_actions[] = {
	{ KEY_ENTER,  NULL,       "View/Edit"},
	{ KEY_F(6),   "Ren",      "Rename"},
	{ KEY_F(8),   "Del",      "Delete"},
	//{ 1, "F1",    "?",        "Help"},
	{ ' ',        NULL,       "Mark/unmark"},
	{ KEY_F(9),   "MarkPat",  "Mark pattern"},
	{ KEY_F(10),  "UnmrkAll", "Unmark all"},
	{ '\t',       NULL,       "> Other pane"},
	{ 'N' & 0x1f, NULL,       "Search again"},
	{ 0x1f,       NULL,       "Quit"},
	{ 0 },
};

static const struct action *type_actions[2] = {
	dir_actions, wad_actions,
};

static const struct action *action_lists[2][2] = {
	{dir_to_dir, dir_to_wad},
	{wad_to_dir, wad_to_wad},
};

static const char *KeyDescription(int key)
{
	static char buf[10];
	switch (key) {
	case KEY_F(1): return "F1";
	case KEY_F(2): return "F2";
	case KEY_F(3): return "F3";
	case KEY_F(4): return "F4";
	case KEY_F(5): return "F5";
	case KEY_F(6): return "F6";
	case KEY_F(7): return "F7";
	case KEY_F(8): return "F8";
	case KEY_F(9): return "F9";
	case KEY_F(10): return "F10";
	case ' ': return "Space";
	case '\t': return "Tab";
	case KEY_ENTER: return "Ent";
	case 0x1f: return "Esc";
	default: break;
	}
	if (key < 0x1f) {
		snprintf(buf, sizeof(buf), "^%c", 'A' + (key & 0x1f) - 1);
		return buf;
	}
	return "?";
}

static void AddActionList(const struct action **result,
                          const struct action *list)
{
	int i, j;

	for (i = 0; list[i].key != 0; i++) {
		for (j = 0; j < MAX_KEY_BINDINGS; j++) {
			if (key_ordering[j] != 0
			 && list[i].key == key_ordering[j]) {
				break;
			}
		}
		assert(j < MAX_KEY_BINDINGS);

		result[j] = &list[i];
	}
}

static void BuildActionsList(const struct action **result, int active, int other)
{
	memset(result, 0, sizeof(struct action *) * MAX_KEY_BINDINGS);

	AddActionList(result, common_actions);
	AddActionList(result, type_actions[active == FILE_TYPE_WAD]);
	AddActionList(result, action_lists[active == FILE_TYPE_WAD]
	                                  [other == FILE_TYPE_WAD]);
}

static void ShowAction(struct actions_pane *p, int y,
                       const struct action *action)
{
	WINDOW *win = p->pane.window;
	char *desc;

	if (action->key == 0) {
		return;
	}
	wattron(win, A_BOLD);
	mvwaddstr(win, y, 2, KeyDescription(action->key));
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
	const struct action *actions[MAX_KEY_BINDINGS], *a;
	WINDOW *win = p->pane.window;
	int i, y;

	BuildActionsList(actions, p->active, p->other);

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	UI_DrawWindowBox(win);
	mvwaddstr(win, 0, 2, " Actions ");

	for (i = 0, y = 1; i < MAX_KEY_BINDINGS; i++) {
		a = actions[i];
		if (a != NULL) {
			ShowAction(p, y, a);
		}
		if (key_ordering[i] == 0 || a != NULL) {
			y++;
		}
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

static int NumShortcuts(const struct action **cells)
{
	int result = 0, i;

	for (i = 0; i < 10; i++) {
		if (cells[i] != NULL) {
			++result;
		}
	}
	return result;
}

static const char *LongName(const struct action *a)
{
	const char *result = a->description;
	if (StringHasPrefix(result, "> ")) {
		result += 2;
	}
	return result;
}

static int SelectShortcutNames(const struct action **cells,
                               const char **names, int columns)
{
	int i;
	int spacing = columns - 2;
	int num_shortcuts = NumShortcuts(cells);

	memset(names, 0, 10 * sizeof(char *));

	for (i = 0; i < 10; ++i) {
		if (cells[i] != NULL) {
			names[i] = cells[i]->shortname;
			spacing -= 1 + strlen(names[i]);
		}
	}

	if (spacing < 0) {
		return 0;
	}

	// Expand some to longer names where possible, but always keep
	// at least one space between shortcuts.
	while (spacing >= num_shortcuts) {
		int best = -1, best_diff = INT_MAX, diff;
		const char *longname;

		for (i = 0; i < 10; i++) {
			if (cells[i] == NULL) {
				continue;
			}
			longname = LongName(cells[i]);
			diff = strlen(longname) - strlen(names[i]);
			if (diff > 0 && diff < best_diff
			 && spacing - diff >= num_shortcuts) {
				best_diff = diff;
				best = i;
			}
		}

		if (best < 0) {
			break;
		}

		names[best] = LongName(cells[best]);
		spacing -= best_diff;
	}

	return spacing / num_shortcuts;
}

static void RecalculateNames(struct actions_bar *p, int columns)
{
	const struct action *actions[MAX_KEY_BINDINGS], *a, *cells[10];
	int i, num_cells = 0;

	BuildActionsList(actions, p->active, p->other);

	memset(cells, 0, sizeof(cells));
	for (i = 0; i < MAX_KEY_BINDINGS; i++) {
		a = actions[i];
		if (a != NULL && a->key >= KEY_F(1) && a->key <= KEY_F(12)) {
			cells[a->key - KEY_F(1)] = a;
			++num_cells;
		}
	}

	p->spacing = SelectShortcutNames(cells, p->names, columns);
	p->last_width = columns;
}

static void DrawActionsBar(void *pane)
{
	struct actions_bar *p = pane;
	WINDOW *win = p->pane.window;
	int i, j;
	int columns = getmaxx(win);

	if (columns != p->last_width) {
		RecalculateNames(p, columns);
	}

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	mvwaddstr(win, 0, 0, "");

	for (i = 0; i < 10; ++i) {
		char buf[10];
		if (p->names[i] == NULL) {
			continue;
		}
		wattron(win, COLOR_PAIR(PAIR_PANE_COLOR));
		for (j = 0; j < p->spacing; j++) {
			waddstr(win, " ");
		}
		wattron(win, A_BOLD);
		snprintf(buf, sizeof(buf), "%d", i + 1);
		waddstr(win, buf);
		wattroff(win, A_BOLD);
		wattron(win, COLOR_PAIR(PAIR_HEADER));
		waddstr(win, p->names[i]);
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
	RecalculateNames(pane, getmaxx(pane->pane.window));
}
