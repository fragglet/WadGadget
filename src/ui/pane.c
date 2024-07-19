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

void UI_TriggerRecalculate(void);
void UI_RecalculateStacks(void);

static struct pane *actions_bar, *title_bar;
static bool main_loop_exited = false;

void UI_PaneKeypress(void *pane, int key)
{
	struct pane *p = pane;

	if (p->keypress != NULL) {
		p->keypress(p, key);
	}
}

static struct pane **GetPanePtr(struct pane_stack *stack, struct pane *p)
{
	struct pane **ptr;

	ptr = &stack->panes;
	for (;;) {
		if (*ptr == p) {
			return ptr;
		}
		if (*ptr == NULL) {
			return NULL;
		}
		ptr = &(*ptr)->next;
	}
}

void UI_PaneShow(void *_pane)
{
	struct pane *pane = _pane;
	struct pane **top_ptr;

	// In case already shown, remove first; we will add it back
	// at the top of the stack.
	UI_PaneHide(pane);

	top_ptr = GetPanePtr(UI_CurrentStack(), NULL);
	pane->next = NULL;
	*top_ptr = pane;
}

int UI_PaneHide(void *_pane)
{
	struct pane *pane = _pane;
	struct pane **pane_ptr = GetPanePtr(UI_CurrentStack(), pane);

	if (pane_ptr != NULL) {
		*pane_ptr = pane->next;
		pane->next = NULL;
		return true;
	}

	return false;
}

void UI_DrawPane(struct pane *p)
{
	if (p->draw != NULL) {
		if (p->draw(p)) {
			wnoutrefresh(p->window);
		}
	}
}

static void DimStack(struct pane_stack *stack)
{
	int x, y;

	for (y = stack->state.top_line;
	     y <= stack->state.top_line + stack->state.lines;
	     y++) {
		for (x = 0; x < COLS; ++x) {
			chtype c = mvwinch(newscr, y, x);
			c = (c | A_DIM) & ~A_BOLD;
			mvwchgat(newscr, y, x, 1, c, PAIR_NUMBER(c), NULL);
		}
	}
}

void UI_DrawAllPanes(void)
{
	struct pane_stack *s;
	int cur_x, cur_y;

	UI_RecalculateStacks();

	wbkgdset(newscr, COLOR_PAIR(PAIR_WHITE_BLACK));
	werase(newscr);

	for (s = UI_AllStacks(); s != NULL; s = s->state.next) {
		struct pane *p;

		for (p = s->panes; p != NULL; p = p->next) {
			UI_SetCurrentStack(s);
			UI_DrawPane(p);
		}

		if (s == UI_ActiveStack()) {
			getyx(newscr, cur_y, cur_x);
		}

		UI_DrawPane(title_bar);

		if (s != UI_ActiveStack()) {
			DimStack(s);
		}
	}

	UI_DrawPane(actions_bar);

	// We move the cursor to its last position in the topmost pane,
	// but ignoring the top and bottom bars.
	move(cur_y, cur_x);

	doupdate();
}

void UI_RaisePaneToTop(void *pane)
{
	if (UI_PaneHide(pane)) {
		UI_PaneShow(pane);
	}
}

static struct pane *GetPrevPane(struct pane_stack *stack, struct pane *pane)
{
	struct pane *p = stack->panes;

	while (p != NULL) {
		if (p->next == pane) {
			return p;
		}
		p = p->next;
	}

	return NULL;
}

void UI_StackKeypress(struct pane_stack *s, int key)
{
	struct pane *p;

	UI_SetCurrentStack(s);
	UI_PaneKeypress(actions_bar, key);

	// Keypress goes to the top pane that has a keypress handler.

	p = GetPrevPane(s, NULL);
	while (p != NULL) {
		UI_SetCurrentStack(s);
		if (p->keypress != NULL) {
			UI_PaneKeypress(p, key);
			break;
		}
		p = GetPrevPane(s, p);
	}
}

void UI_InputKeypress(int key)
{
	if (key == CTRL_('L')) {
		clearok(stdscr, TRUE);
		wrefresh(stdscr);
		return;
	}
	if (key == KEY_RESIZE) {
		UI_TriggerRecalculate();
		return;
	}

	UI_StackKeypress(UI_ActiveStack(), key);
}

static bool HandleKeypress(void)
{
	int key;

	// TODO: This should handle multiple keypresses before returning.
	key = getch();
	if (key == ERR) {
		return false;
	}

	UI_InputKeypress(key);

	return true;
}

static void HandleKeypresses(void)
{
	// Block on the first keypress.
	nodelay(stdscr, 0);
	HandleKeypress();

	// We now need to do at least one screen update. But read any
	// additional keypresses first.
	for (;;) {
		nodelay(stdscr, 1);
		if (!HandleKeypress()) {
			break;
		}
	}
}

void UI_RunMainLoop(void)
{
	while (!main_loop_exited) {
		UI_DrawAllPanes();
		HandleKeypresses();
	}

	main_loop_exited = false;
}

void UI_ExitMainLoop(void)
{
	main_loop_exited = true;
}

void UI_Init(void)
{
	actions_bar = UI_ActionsBarInit();
	title_bar = UI_TitleBarInit();
}
