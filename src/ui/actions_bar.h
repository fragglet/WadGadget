//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI__ACTIONS_BAR_H_INCLUDED
#define UI__ACTIONS_BAR_H_INCLUDED

#include <stdbool.h>

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
void UI_ActionsBarSetFunctionKeys(bool function_keys);
void UI_ActionsBarRecalculate(void);

const char *UI_ActionKeyDescription(const struct action *a, bool function_keys);

#endif /* #ifndef UI__ACTIONS_BAR_H_INCLUDED */
