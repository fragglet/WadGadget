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
#include <stdbool.h>
#include <limits.h>

#include "ui/actions_bar.h"
#include "ui/colors.h"
#include "common.h"
#include "stringlib.h"
#include "ui/ui.h"

struct actions_accel {
	const char *name;
	char key[8];
	const struct action *action;
};

struct actions_bar {
	struct pane pane;
	struct actions_accel accels[MAX_KEY_BINDINGS];
	int last_width, spacing;
	const struct action **actions;
	bool function_keys;
};

static struct actions_bar actions_bar_singleton;

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

static void ExpandAcceleratorNames(struct actions_bar *p, int *spacing)
{
	struct actions_accel *accel;
	int i, num_accels = 0;

	for (i = 0; i < arrlen(p->accels); i++) {
		if (p->accels[i].action != NULL) {
			++num_accels;
		}
	}

	// Expand some to longer names where possible, but always keep at
	// least one space between shortcuts.
	while (*spacing > num_accels) {
		int best = -1, best_diff = INT_MAX, diff;
		const char *longname;

		for (i = 0; i < arrlen(p->accels) && *spacing > num_accels; i++) {
			accel = &p->accels[i];
			if (accel->action == NULL) {
				continue;
			}
			longname = LongName(accel->action);
			diff = strlen(longname) - strlen(p->accels[i].name);
			if (diff > 0 && diff < best_diff
			 && *spacing - diff >= num_accels) {
				best_diff = diff;
				best = i;
			}
		}

		if (best < 0) {
			break;
		}

		p->accels[best].name = LongName(p->accels[best].action);
		*spacing -= best_diff;
	}
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
			         UI_ActionKeyDescription(a, p->function_keys));
		}
		spacing -= strlen(accel->key) + strlen(accel->name);
	}

	if (spacing < 0) {
		return 0;
	}

	// In non-shortcut mode, we always use short names so we can cram in as
	// many shortcuts as possible.
	if (p->function_keys) {
		ExpandAcceleratorNames(p, &spacing);
	}

	// Can we fit any more shortcuts in?
	i = 0;
	add_index = 10;
	while (p->actions != NULL && p->actions[i] != NULL
	    && add_index < MAX_KEY_BINDINGS) {
		a = p->actions[i];
		// Don't add any function key actions; we already have them.
		if (a == NULL || a->shortname == NULL || HasFunctionKey(a)) {
			i++;
			continue;
		}
		accel = &p->accels[add_index];
		snprintf(accel->key, sizeof(accel->key), "%s",
		         UI_ActionKeyDescription(a, false));
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

	ExpandAcceleratorNames(p, &spacing);

	return min(num_shortcuts > 0 ? spacing / num_shortcuts : 1, 3);
}

static void RecalculateNames(struct actions_bar *p, int columns)
{
	const struct action *a, *cells[10];
	int i;

	memset(cells, 0, sizeof(cells));
	for (i = 0; p->actions != NULL && p->actions[i] != NULL; i++) {
		a = p->actions[i];
		if (a != NULL && HasFunctionKey(a)) {
			cells[a->key - KEY_F(1)] = a;
		}
	}

	p->spacing = SetAccelerators(p, cells, columns);
	p->last_width = columns;
}

static bool DrawActionsBar(void *pane)
{
	struct actions_bar *p = pane;
	WINDOW *win = p->pane.window;
	int i, j;

	if (!UI_CurrentStack()->actions_bar_enabled) {
		return false;
	}

	wresize(p->pane.window, 1, COLS);
	mvwin(p->pane.window, LINES - 1, 0);

	if (COLS != p->last_width) {
		RecalculateNames(p, COLS);
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

	return true;
}

// This function is a hack to support the Ins and Del keys, plus some
// other weirder ones, if by some miracle they're ever encountered.
static int TranslateSpecialKey(int key)
{
	switch (key) {
	case KEY_HELP:    return KEY_F(1);        // help
	case KEY_MOVE:    return KEY_F(2);        // move
	case KEY_SMOVE:   return SHIFT_KEY_F(2);  // shift-move
	case KEY_COPY:    return KEY_F(5);        // copy
	case KEY_SCOPY:   return SHIFT_KEY_F(5);  // shift-copy
	case KEY_IC:      return KEY_F(7);        // insert
	case KEY_DC:      return KEY_F(8);        // delete
	case KEY_SDC:     return SHIFT_KEY_F(8);  // shift-delete
	case KEY_CREATE:  return KEY_F(9);        // create
	case KEY_EXIT:    return 27;              // exit
	case KEY_MARK:    return ' ';             // mark
	case KEY_NEXT:    return CTRL_('N');      // next
	case KEY_UNDO:    return CTRL_('Z');      // undo
	case KEY_REDO:    return CTRL_('Y');      // redo
	case KEY_REFRESH: return CTRL_('R');      // refresh
	case KEY_BEG:     return KEY_HOME;        // begin
	default:          return key;
	}
}

static void HandleKeypress(void *_p, int key)
{
	struct actions_bar *p = _p;
	int i;

	if (p->actions == NULL) {
		return;
	}

	key = TranslateSpecialKey(key);

	for (i = 0; p->actions[i] != NULL; i++) {
		if (p->actions[i]->callback != NULL
		 && (key == p->actions[i]->key
		  || key == CTRL_(p->actions[i]->ctrl_key))) {
			p->actions[i]->callback();
			return;
		}
	}
}

struct pane *UI_ActionsBarInit(void)
{
	actions_bar_singleton.pane.window = newwin(1, COLS, LINES - 1, 0);
	actions_bar_singleton.pane.draw = DrawActionsBar;
	actions_bar_singleton.pane.keypress = HandleKeypress;
	actions_bar_singleton.actions = NULL;
	actions_bar_singleton.function_keys = true;

	return &actions_bar_singleton.pane;
}

const struct action **UI_ActionsBarSetActions(const struct action **actions)
{
	const struct action **old_actions;

	old_actions = actions_bar_singleton.actions;
	actions_bar_singleton.actions = actions;
	RecalculateNames(&actions_bar_singleton, COLS);

	return old_actions;
}

void UI_ActionsBarSetFunctionKeys(bool function_keys)
{
	actions_bar_singleton.function_keys = function_keys;
	actions_bar_singleton.last_width = -1;
}

bool UI_ActionsBarEnable(bool enabled)
{
	struct pane_stack *stack = UI_CurrentStack();
	bool result = stack->actions_bar_enabled;
	stack->actions_bar_enabled = enabled;
	return result;
}

const char *UI_ActionKeyDescription(const struct action *a, bool function_keys)
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
