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

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void UI_ActionsPaneSet(struct actions_pane *pane,
                       const struct action **actions, bool left_to_right,
                       bool function_keys);
