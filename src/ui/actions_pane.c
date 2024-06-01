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

#include "ui/actions_pane.h"
#include "ui/colors.h"
#include "common.h"
#include "stringlib.h"
#include "ui/ui.h"

static const int key_ordering[] = {
	KEY_F(1), KEY_F(2), KEY_F(3), KEY_F(4), KEY_F(5), KEY_F(6),
	KEY_F(7), KEY_F(8), KEY_F(9), ' ', CTRL_('G'), KEY_F(10),
	0,
	'\t', '\r', CTRL_('D'),
	CTRL_('Z'), CTRL_('Y'),
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

static bool HasFunctionKey(const struct action *a)
{
	return a->key >= KEY_F(1) && a->key <= KEY_F(10);
}

static int SetAccelerators(struct actions_bar *p, const struct action **cells,
                           int columns)
{
	const struct action *a;
	struct actions_accel *accel;
	int i, add_index, diff;
	int spacing = columns - 2;
	int num_shortcuts = NumShortcuts(cells);

	memset(p->accels, 0, sizeof(p->accels));

	for (i = 0; i < 10; ++i) {
		a = cells[i];
		if (a == NULL || a->shortname == NULL) {
			continue;
		}
		accel = &p->accels[i];
		accel->action = a;
		accel->name = a->shortname;
		if (p->function_keys) {
			snprintf(accel->key, sizeof(accel->key), "%d", i + 1);
		} else {
			snprintf(accel->key, sizeof(accel->key), "%s",
			         KeyDescription(a, p->function_keys));
		}
		spacing -= strlen(accel->key) + strlen(accel->name);
	}

	if (spacing < 0) {
		return 0;
	}

	// Expand some to longer names where possible, but always keep at
	// least one space between shortcuts. In non-shortcut mode, we
	// always use short names so we can cram in as many shortcuts as
	// possible.
	while (p->function_keys && spacing >= num_shortcuts) {
		int best = -1, best_diff = INT_MAX;
		const char *longname;

		for (i = 0; i < 10; i++) {
			if (cells[i] == NULL) {
				continue;
			}
			longname = LongName(cells[i]);
			diff = strlen(longname) - strlen(p->accels[i].name);
			if (diff > 0 && diff < best_diff
			 && spacing - diff >= num_shortcuts) {
				best_diff = diff;
				best = i;
			}
		}
		if (best < 0) {
			break;
		}

		p->accels[best].name = LongName(cells[best]);
		spacing -= best_diff;
	}

	// Can we fit any more shortcuts in?
	i = 0;
	add_index = 10;
	while (p->actions[i] != NULL && add_index < MAX_KEY_BINDINGS) {
		a = p->actions[i];
		// Don't add any function key actions; we already have them.
		if (a == NULL || a->shortname == NULL || HasFunctionKey(a)) {
			i++;
			continue;
		}
		accel = &p->accels[add_index];
		snprintf(accel->key, sizeof(accel->key), "%s",
		         KeyDescription(a, false));
		// Only add the action if it fits and will still leave at
		// least one space between accelerators.
		diff = strlen(a->shortname) + strlen(accel->key);
		if (diff < spacing - num_shortcuts) {
			accel->name = a->shortname;
			accel->action = a;
			spacing -= diff;
			++add_index;
			++num_shortcuts;
		}
		i++;
	}

	return spacing / num_shortcuts;
}

static void RecalculateNames(struct actions_bar *p, int columns)
{
	const struct action *a, *cells[10];
	int i;

	if (p->actions == NULL) {
		return;
	}

	memset(cells, 0, sizeof(cells));
	for (i = 0; p->actions[i] != NULL; i++) {
		a = p->actions[i];
		if (a != NULL && HasFunctionKey(a)) {
			cells[a->key - KEY_F(1)] = a;
		}
	}

	p->spacing = SetAccelerators(p, cells, columns);
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

	for (i = 0; i < MAX_KEY_BINDINGS; ++i) {
		if (p->accels[i].action == NULL) {
			continue;
		}
		wattron(win, COLOR_PAIR(PAIR_PANE_COLOR));
		for (j = 0; j < p->spacing; j++) {
			waddstr(win, " ");
		}
		wattron(win, A_BOLD);
		waddstr(win, p->accels[i].key);
		wattroff(win, A_BOLD);
		wattron(win, COLOR_PAIR(PAIR_HEADER));
		waddstr(win, p->accels[i].name);
	}
}

void UI_ActionsBarInit(struct actions_bar *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsBar;
	pane->pane.keypress = NULL;
	pane->actions = NULL;
	pane->function_keys = true;
}

void UI_ActionsBarSet(struct actions_bar *pane,
                      const struct action **actions,
                      bool function_keys)
{
	pane->actions = actions;
	pane->function_keys = function_keys;
	RecalculateNames(pane, getmaxx(pane->pane.window));
}
