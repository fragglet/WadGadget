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

static const int key_ordering[] = {
	KEY_F(1), KEY_F(2), KEY_F(3), KEY_F(4), KEY_F(5), KEY_F(6),
	KEY_F(7), KEY_F(8), KEY_F(9), ' ', CTRL_('G'), KEY_F(10),
	0,
	'\t', '\r', CTRL_('D'),
	27, CTRL_('J'),
};

static const char *KeyDescription(const struct action *a, bool function_keys)
{
	int key = a->key;
	if (!function_keys && a->key >= KEY_F(1) && a->key <= KEY_F(10)
	 && a->ctrl_key != 0) {
		key = 0;
	}
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
	case '\r': return "Ent";
	case 27: return "Esc";
	default: break;
	}
	if (a->ctrl_key) {
		static char buf[3];
		snprintf(buf, sizeof(buf), "^%c", a->ctrl_key);
		return buf;
	}
	return "??";
}

static int ShowAction(struct actions_pane *p, int y,
                      const struct action *action, bool right_ok)
{
	WINDOW *win = p->pane.window;
	bool arrows = false, ellipsis = false, right_side = false;
	int x, result;
	char *desc;

	if (action->key == 0 && action->ctrl_key == 0) {
		return 0;
	}

	for (desc = action->description;; ++desc) {
		if (*desc == '>') {
			arrows = true;
		} else if (*desc == '|') {
			right_side = true;
		} else if (*desc == '.') {
			ellipsis = true;
		} else if (*desc != ' ') {
			break;
		}
	}

	if (right_side && right_ok) {
		x = 14;
		--y;
		result = 0;
	} else {
		x = 2;
		result = 1;
	}

	wattron(win, A_BOLD);
	mvwaddstr(win, y, x, KeyDescription(action, p->function_keys));
	wattroff(win, A_BOLD);
	waddstr(win, " - ");

	if (arrows && !p->left_to_right) {
		wattron(win, A_BOLD);
		waddstr(win, "<<< ");
		wattroff(win, A_BOLD);
	}
	waddstr(win, desc);
	if (ellipsis) {
		waddstr(win, "...");
	}
	if (arrows && p->left_to_right) {
		wattron(win, A_BOLD);
		waddstr(win, " >>>");
		wattroff(win, A_BOLD);
	}

	return result;
}

static void DrawActionsPane(void *pane)
{
	struct actions_pane *p = pane;
	const struct action *a;
	WINDOW *win = p->pane.window;
	int i, y, last_idx = -1;

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	UI_DrawWindowBox(win);
	mvwaddstr(win, 0, 2, " Actions ");

	for (i = 0, y = 1; i < MAX_KEY_BINDINGS; i++) {
		a = p->actions[i];
		if (a != NULL) {
			y += ShowAction(p, y, a, last_idx == i - 1);
		}
		if (key_ordering[i] == 0) {
			y++;
		}
		last_idx = i;
	}
}

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsPane;
	pane->pane.keypress = NULL;
	pane->function_keys = true;
	memset(pane->actions, 0, sizeof(pane->actions));
}

void UI_ActionsPaneSet(struct actions_pane *pane,
                       const struct action **actions, bool left_to_right,
                       bool function_keys)
{
	const struct action *a;
	int i, j;

	pane->left_to_right = left_to_right;
	pane->function_keys = function_keys;
	memset(pane->actions, 0, sizeof(pane->actions));

	for (i = 0; actions[i] != NULL; i++) {
		a = actions[i];
		for (j = 0; j < arrlen(key_ordering); ++j) {
			if (key_ordering[j] != 0
			 && (key_ordering[j] == a->key
			  || key_ordering[j] == CTRL_(a->ctrl_key))) {
				pane->actions[j] = a;
				break;
			}
		}
	}
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
	while (strchr(" >|.", *result)) {
		++result;
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
	const struct action *a, *cells[10];
	int i, num_cells = 0;

	if (p->actions == NULL) {
		return;
	}

	memset(cells, 0, sizeof(cells));
	for (i = 0; p->actions[i] != NULL; i++) {
		a = p->actions[i];
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
	pane->actions = NULL;
}

void UI_ActionsBarSet(struct actions_bar *pane,
                      const struct action **actions)
{
	pane->actions = actions;
	RecalculateNames(pane, getmaxx(pane->pane.window));
}
