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
#include "pane.h"
#include "vfs.h"

#define MAX_KEY_BINDINGS 20

struct actions_pane {
	struct pane pane;
	int left_to_right;
	const struct action *actions[MAX_KEY_BINDINGS];
};

struct actions_bar {
	struct pane pane;
	const char *names[10];
	int last_width, spacing;
	const struct action **actions;
};

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void UI_ActionsPaneSet(struct actions_pane *pane,
                       const struct action **actions, int left_to_right);

void UI_ActionsBarInit(struct actions_bar *pane, WINDOW *win);
void UI_ActionsBarSet(struct actions_bar *pane,
                      const struct action **actions);
