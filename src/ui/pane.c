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
#include "ui/title_bar.h"

#define MAX_SCREEN_PANES 10

static struct pane_stack main_stack;
static struct pane_stack *current_stack = &main_stack;
static struct pane *actions_bar, *title_bar;
static bool main_loop_exited = false;

void UI_PaneKeypress(void *pane, int key)
{
	struct pane *p = pane;

	if (p->keypress != NULL) {
		p->keypress(p, key);
	}
}

static struct pane **GetPanePtr(struct pane *p)
{
	struct pane **ptr;

	ptr = &current_stack->panes;
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

	top_ptr = GetPanePtr(NULL);
	pane->next = NULL;
	*top_ptr = pane;
}

int UI_PaneHide(void *_pane)
{
	struct pane *pane = _pane;
	struct pane **pane_ptr = GetPanePtr(pane);

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

static void DrawStack(struct pane_stack *stack)
{
	struct pane *p;

	for (p = stack->panes; p != NULL; p = p->next) {
		UI_DrawPane(p);
	}
}

void UI_DrawAllPanes(void)
{
	int cur_x, cur_y;

	wbkgdset(newscr, COLOR_PAIR(PAIR_WHITE_BLACK));
	werase(newscr);

	DrawStack(current_stack);

	getyx(newscr, cur_y, cur_x);
	UI_DrawPane(actions_bar);
	UI_DrawPane(title_bar);

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

static struct pane *GetPrevPane(struct pane *pane)
{
	struct pane *p = current_stack->panes;

	while (p != NULL) {
		if (p->next == pane) {
			return p;
		}
		p = p->next;
	}

	return NULL;
}

static void InputKeyPress(int key)
{
	struct pane *p;

	if (key == CTRL_('L')) {
		clearok(stdscr, TRUE);
		wrefresh(stdscr);
		return;
	}

	UI_PaneKeypress(actions_bar, key);

	// Keypress goes to the top pane that has a keypress handler.
	for (p = GetPrevPane(NULL); p != NULL; p = GetPrevPane(p)) {
		if (p->keypress != NULL) {
			UI_PaneKeypress(p, key);
			break;
		}
	}
}

static bool HandleKeypress(void)
{
	int key;

	// TODO: This should handle multiple keypresses before returning.
	key = getch();
	if (key == ERR) {
		return false;
	}

	InputKeyPress(key);

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

struct pane *UI_SavePanes(void)
{
	struct pane *result = current_stack->panes;
	current_stack->panes = NULL;
	return result;
}

void UI_RestorePanes(struct pane *old_panes)
{
	while (current_stack->panes != NULL) {
		UI_PaneHide(current_stack->panes);
	}

	current_stack->panes = old_panes;
	InputKeyPress(KEY_RESIZE);
}

struct pane_stack *UI_CurrentStack(void)
{
	return current_stack;
}

void UI_SaveScreen(struct pane_stack *ss)
{
	ss->panes = UI_SavePanes();
	ss->actions = UI_ActionsBarSetActions(NULL);
	ss->actions_bar_enabled = UI_ActionsBarEnable(true);
	ss->title = UI_SetTitleBar(NULL);
	ss->subtitle = UI_SetSubtitle(NULL);
}

void UI_RestoreScreen(struct pane_stack *ss)
{
	UI_RestorePanes(ss->panes);
	UI_ActionsBarSetActions(ss->actions);
	UI_ActionsBarEnable(ss->actions_bar_enabled);
	UI_SetTitleBar(ss->title);
	UI_SetSubtitle(ss->subtitle);
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
