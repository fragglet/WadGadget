//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

struct pane_stack;

struct pane_stack_state {
	int top_line, bottom_line;
	struct pane_stack *next;
};

struct pane_stack {
	int lines;
	struct pane *panes;
	const struct action **actions;
	bool actions_bar_enabled;
	const char *title, *subtitle;
	struct pane_stack_state state;
};

void UI_SetActiveStack(struct pane_stack *stack);
struct pane_stack *UI_ActiveStack(void);
void UI_SetCurrentStack(struct pane_stack *stack);
struct pane_stack *UI_CurrentStack(void);
struct pane_stack *UI_NewStack(void);
void UI_FreeStack(struct pane_stack *stack);
void UI_AddStack(struct pane_stack *stack);
void UI_RemoveStack(struct pane_stack *stack);
struct pane_stack *UI_AllStacks(void);

void UI_GetDesktopLines(int *start, int *end);

const struct action **UI_ActionsBarSetActions(const struct action **actions);
void UI_ActionsBarEnable(bool enabled);
void UI_SetTitleBar(const char *msg);
void UI_SetSubtitle(const char *msg);

#endif /* #ifndef STACK_H_INCLUDED */
