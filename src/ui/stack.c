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

// main_stack is the first stack that is set when the program is started.
static struct pane_stack main_stack;

// Linked list of all stacks being shown on the screen.
static struct pane_stack *stacks = &main_stack;

// The active stack is the one that the user is currently interacting
// with; keypresses will go to this stack.
static struct pane_stack *active_stack = &main_stack;

// The "current" stack is the one that is currently being drawn or receiving
// keypresses; this changes continually during rendering etc.
static struct pane_stack *current_stack = &main_stack;

void UI_RecalculateStacks(void)
{
	struct pane_stack *s, *old = current_stack;
	int flex_lines = LINES, num_flex = 0;
	int lines = LINES, top_line;

	if (active_stack->actions_bar_enabled) {
		--flex_lines;
		--lines;
	}

	for (s = stacks; s != NULL; s = s->state.next) {
		if (s->lines == 0) {
			++num_flex;
		} else {
			flex_lines -= s->lines;
		}
	}

	top_line = 0;
	for (s = stacks; s != NULL; s = s->state.next) {
		s->state.top_line = top_line;
		if (s->lines == 0) {
			top_line += flex_lines / num_flex;
			lines -= flex_lines / num_flex;
		} else {
			top_line += s->lines;
			lines -= s->lines;
		}
		s->state.bottom_line = top_line - 1;
		// Last stack gets all remaining lines.
		if (s->state.next == NULL) {
			s->state.bottom_line += lines;
		}
	}

	for (s = stacks; s != NULL; s = s->state.next) {
		UI_StackKeypress(s, KEY_RESIZE);
	}

	current_stack = old;
}

struct pane_stack *UI_ActiveStack(void)
{
	return active_stack;
}

void UI_SetCurrentStack(struct pane_stack *stack)
{
	current_stack = stack;
}

struct pane_stack *UI_CurrentStack(void)
{
	if (current_stack->state.bottom_line == 0) {
		UI_RecalculateStacks();
	}
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

void UI_GetDesktopLines(int *start, int *end)
{
	if (current_stack->state.bottom_line == 0) {
		UI_RecalculateStacks();
	}
	*start = current_stack->state.top_line + 1;
	*end = current_stack->state.bottom_line;
}

void UI_AddStack(struct pane_stack *stack)
{
	stack->state.next = stacks;
	stacks = stack;
	active_stack = stack;
	current_stack = stack;
	UI_RecalculateStacks();
	UI_ActionsBarRecalculate();
}

void UI_RemoveStack(struct pane_stack *stack)
{
	struct pane_stack **s;

	for (s = &stacks; *s != NULL; s = &(*s)->state.next) {
		if (*s == stack) {
			*s = stack->state.next;
			break;
		}
	}
	if (active_stack == stack) {
		active_stack = stacks;
	}
	if (current_stack == stack) {
		current_stack = stacks;
	}
	UI_RecalculateStacks();
	UI_ActionsBarRecalculate();
}

struct pane_stack *UI_AllStacks(void)
{
	return stacks;
}

const struct action **UI_ActionsBarSetActions(const struct action **actions)
{
	const struct action **old_actions = current_stack->actions;
	if (current_stack->actions == actions) {
		return actions;
	}
	current_stack->actions = actions;
	if (current_stack == active_stack) {
		UI_RecalculateStacks();
		UI_ActionsBarRecalculate();
	}

	return old_actions;
}

void UI_ActionsBarEnable(bool enabled)
{
	if (current_stack->actions_bar_enabled == enabled) {
		return;
	}
	current_stack->actions_bar_enabled = enabled;
	if (current_stack == active_stack) {
		UI_RecalculateStacks();
		UI_ActionsBarRecalculate();
	}
}

void UI_SetTitleBar(const char *msg)
{
	current_stack->title = msg;
}

void UI_SetSubtitle(const char *msg)
{
	current_stack->subtitle = msg;
}
