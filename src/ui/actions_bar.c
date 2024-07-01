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
	bool enabled, function_keys;
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

	return num_shortcuts > 0 ? spacing / num_shortcuts : 1;
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

	if (!p->enabled) {
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

struct pane *UI_ActionsBarInit(void)
{
	actions_bar_singleton.pane.window = newwin(1, COLS, LINES - 1, 0);
	actions_bar_singleton.pane.draw = DrawActionsBar;
	actions_bar_singleton.pane.keypress = NULL;
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
}

bool UI_ActionsBarEnable(bool enabled)
{
	bool result = actions_bar_singleton.enabled;
	actions_bar_singleton.enabled = enabled;
	return result;
}
