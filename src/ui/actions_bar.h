//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI_ACTIONS_BAR_H
#define UI_ACTIONS_BAR_H

#include "actions.h"

#define MAX_KEY_BINDINGS 30

struct action {
	// If non-zero, key that when pressed invokes the callback.
	int key;
	// If non-zero, ctrl-[this] invokes the callback.
	int ctrl_key;
	// Abbreviated name shown on the actions bar in Commander Mode.
	// If NULL, this action is never shown on the actions bar.
	char *shortname;
	// Long name shown on the actions pane in Normal Mode , or in
	// Commander Mode if there is space.
	char *description;
	void (*callback)(void);
};

struct pane *UI_ActionsBarInit(void);
const struct action **UI_ActionsBarSetActions(const struct action **actions);
void UI_ActionsBarSetFunctionKeys(bool function_keys);
bool UI_ActionsBarEnable(bool enabled);

#endif  /* #ifndef UI_ACTIONS_BAR_H */
