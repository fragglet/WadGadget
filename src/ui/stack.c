//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "common.h"
#include "ui/actions_bar.h"
#include "ui/colors.h"
#include "ui/pane.h"
#include "ui/stack.h"
#include "ui/title_bar.h"

#define MAX_SCREEN_PANES 10

static struct pane_stack main_stack;
static struct pane_stack *current_stack = &main_stack;

struct pane_stack *UI_CurrentStack(void)
{
	return current_stack;
}

struct pane_stack *UI_NewStack(void)
{
	return checked_calloc(1, sizeof(struct pane_stack));
}

void UI_FreeStack(struct pane_stack *stack)
{
	struct pane **p = &stack->panes;

	while (*p != NULL) {
		struct pane **next = &(*p)->next;
		*p = NULL;
		p = next;
	}
}

struct pane_stack *UI_SwapStack(struct pane_stack *stack)
{
	struct pane_stack *result = current_stack;
	current_stack = stack;
	UI_ActionsBarSetActions(stack->actions);
	UI_InputKeyPress(KEY_RESIZE);
	return result;
}

void UI_GetDesktopLines(int *start, int *end)
{
	*start = 1;
	*end = LINES - 1;

	// If actions bar is enabled, it needs a line.
	if (UI_ActionsBarEnable(false)) {
		UI_ActionsBarEnable(true);
		--*end;
	}
}
