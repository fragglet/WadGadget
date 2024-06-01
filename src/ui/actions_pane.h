//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "actions.h"
#include "ui/pane.h"
#include "fs/vfs.h"

#define MAX_KEY_BINDINGS 30

struct actions_pane {
	struct pane pane;
	bool left_to_right;
	bool function_keys;
	const struct action *actions[MAX_KEY_BINDINGS];
};

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

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void UI_ActionsPaneSet(struct actions_pane *pane,
                       const struct action **actions, bool left_to_right,
                       bool function_keys);

void UI_ActionsBarInit(struct actions_bar *pane, WINDOW *win);
void UI_ActionsBarSet(struct actions_bar *pane,
                      const struct action **actions, bool function_keys);
