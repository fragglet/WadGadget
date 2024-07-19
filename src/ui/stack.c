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

static bool need_recalculate = true;

void UI_RecalculateStacks(void)
{
	struct pane_stack *s, *last_flex = NULL, *old = current_stack;
	int flex_lines = LINES, num_flex = 0;
	int lines = LINES, top_line;

	if (!need_recalculate) {
		return;
	}

	for (s = stacks; s != NULL; s = s->state.next) {
		if (s->lines == 0) {
			++num_flex;
			last_flex = s;
		} else {
			flex_lines -= s->lines;
		}
	}

	top_line = 0;
	for (s = stacks; s != NULL; s = s->state.next) {
		s->state.top_line = top_line;
		if (s->lines == 0) {
			s->state.lines = flex_lines / num_flex;
		} else {
			s->state.lines = s->lines;
		}
		top_line += s->state.lines;
		lines -= s->state.lines;
		// Last stack gets all remaining lines.
		if (s->state.next == NULL) {
			s->state.lines += lines;
		}
	}

	for (s = stacks; s != NULL; s = s->state.next) {
		UI_StackKeypress(s, KEY_RESIZE);
	}

	// We calculated with the assumption that the actions bar was not
	// present. If the active pane decided it needed it, we shrink the
	// bottom stack by one line and send another resize event.
	if (active_stack->actions_bar_enabled && last_flex != NULL) {
		last_flex->state.lines--;
		UI_StackKeypress(last_flex, KEY_RESIZE);
	}

	current_stack = old;
	need_recalculate = false;

	if (active_stack->actions_bar_enabled) {
		UI_ActionsBarRecalculate();
	}
}

void UI_TriggerRecalculate(void)
{
	need_recalculate = true;
}

void UI_SetActiveStack(struct pane_stack *stack)
{
	active_stack = stack;
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
	return current_stack;
}

struct pane_stack *UI_NewStack(void)
{
	return checked_calloc(1, sizeof(struct pane_stack));
}

void UI_SetFullscreenStack(struct pane_stack *stack)
{
	stack->state.next = NULL;
	stack->state.suspended_stacks = stacks;
	stacks = stack;
	active_stack = stack;
	current_stack = stack;
	need_recalculate = true;
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
	*start = current_stack->state.top_line + 1;
	*end = current_stack->state.top_line
	     + current_stack->state.lines - 1;
}

void UI_AddStack(struct pane_stack *stack)
{
	stack->state.next = stacks;
	stacks = stack;
	active_stack = stack;
	current_stack = stack;
	need_recalculate = true;
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

	stack->state.next = NULL;

	if (stack->state.suspended_stacks != NULL) {
		if (stacks == NULL) {
			// This stack was activated as a fullscreen stack, so
			// bring back the old stacks.
			stacks = stack->state.suspended_stacks;
		} else {
			// If other stacks are still open, don't bring them
			// back, but pass on to another stack. Once all are
			// closed they'll be brought back.
			stacks->state.suspended_stacks =
				stack->state.suspended_stacks;
		}
		stack->state.suspended_stacks = NULL;
	}


	if (stack == active_stack) {
		active_stack = stacks;
	}
	if (stack == current_stack) {
		current_stack = stacks;
	}
	need_recalculate = true;
}

struct pane_stack *UI_AllStacks(void)
{
	return stacks;
}

const struct action **UI_ActionsBarSetActions(const struct action **actions)
{
	const struct action **old_actions = current_stack->actions;
	current_stack->actions = actions;
	if (current_stack == active_stack) {
		need_recalculate = true;
	}

	return old_actions;
}

void UI_ActionsBarEnable(bool enabled)
{
	current_stack->actions_bar_enabled = enabled;
	if (current_stack == active_stack) {
		need_recalculate = true;
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
