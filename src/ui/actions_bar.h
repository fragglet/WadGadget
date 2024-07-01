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

#define MAX_KEY_BINDINGS 30

struct pane *UI_ActionsBarInit(void);
const struct action **UI_ActionsBarSetActions(const struct action **actions);
void UI_ActionsBarSetFunctionKeys(bool function_keys);
bool UI_ActionsBarEnable(bool enabled);
