//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "ui/actions_bar.h"
#include "ui/pane.h"

struct actions_pane {
	struct pane pane;
	bool left_to_right;
	bool function_keys;
	const struct action *actions[MAX_KEY_BINDINGS];
};

void B_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void B_ActionsPaneSet(struct actions_pane *pane,
                      const struct action **actions, bool left_to_right,
                      bool function_keys);
