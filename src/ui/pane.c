//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "ui/pane.h"

#include <stdlib.h>
#include <stdbool.h>

#include "common.h"
#include "ui/actions_bar.h"
#include "ui/colors.h"
#include "ui/stack.h"
#include "ui/title_bar.h"
#include "ui/ui.h"

#define MAX_SCREEN_PANES 10

void UI_TriggerRecalculate(void);
void UI_RecalculateStacks(void);

static struct pane_stack *mouse_cur_stack;
static struct pane *mouse_cur_pane;
static int mouse_cur_x, mouse_cur_y;
static doubleclick_continuation mouse_click_continuation;

static struct pane *actions_bar, *title_bar;
static bool main_loop_exited = false;

void UI_PaneKeypress(void *pane, int key)
{
	struct pane *p = pane;

	if (p->keypress != NULL) {
		p->keypress(p, key);
	}
}

doubleclick_continuation UI_PaneMouseClick(void *pane, int x, int y)
{
	struct pane *p = pane;

	if (p->mouse_click != NULL) {
		return p->mouse_click(p, x, y);
	}

	return NULL;
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
			UI_DimScreenArea(0, s->state.top_line,
			                 COLS, s->state.lines, -1);
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

static bool CheckMouseInPane(MEVENT *ev, struct pane *p)
{
	int px, py, pw, ph;
	int relx, rely;

	getbegyx(p->window, py, px);
	getmaxyx(p->window, ph, pw);

	if (ev->x < px || ev->x >= px + pw
	 || ev->y < py || ev->y >= py + ph) {
		return false;

	}

	relx = ev->x - px;
	rely = ev->y - py;

	// Double clicks are only valid if the clicks go to the
	// same screen location each time.
	if (mouse_click_continuation != NULL
	 && (p != mouse_cur_pane || relx != mouse_cur_x
	  || rely != mouse_cur_y)) {
		mouse_click_continuation = NULL;
	}

	mouse_cur_pane = p;
	mouse_cur_x = relx;
	mouse_cur_y = rely;
	return true;
}

static bool UpdateMousePosition(MEVENT *ev)
{
	struct pane_stack *s;

	if (UI_ActiveStack()->actions_bar_enabled
	 && CheckMouseInPane(ev, actions_bar)) {
		mouse_cur_stack = UI_ActiveStack();
		return true;
	}

	// Walk through all panes until we find the first pane that contains
	// the current mouse cursor position.
	for (s = UI_AllStacks(); s != NULL; s = s->state.next) {
		struct pane *p;

		for (p = GetPrevPane(s, NULL);
		     p != NULL; p = GetPrevPane(s, p)) {
			if (CheckMouseInPane(ev, p)) {
				mouse_cur_stack = s;
				return true;
			}
		}
	}

	// No current pane
	mouse_cur_stack = NULL;
	mouse_cur_pane = NULL;
	return false;
}

static void HandleMouseClick(void)
{
	if (mouse_click_continuation != NULL) {
		// Handle as a doubleclick by calling the continuation.
		// TODO: We should enforce a time limit between clicks
		mouse_click_continuation(mouse_cur_pane);
		mouse_click_continuation = NULL;
	} else {
		mouse_click_continuation = UI_PaneMouseClick(
			mouse_cur_pane, mouse_cur_x, mouse_cur_y);
	}
}

static void HandleMouseEvent(void)
{
	struct pane_stack *s = UI_ActiveStack();
	MEVENT ev;

	if (getmouse(&ev) != OK || !UpdateMousePosition(&ev)) {
		return;
	}
	if (s->exclusive_focus != NULL && mouse_cur_pane != s->exclusive_focus) {
		return;
	}

	UI_SetCurrentStack(mouse_cur_stack);
	switch (ev.bstate) {
	case BUTTON1_PRESSED:
		HandleMouseClick();
		break;
	case BUTTON4_PRESSED:
		UI_PaneKeypress(mouse_cur_pane, KEY_UP);
		break;
	case BUTTON5_PRESSED:
		UI_PaneKeypress(mouse_cur_pane, KEY_DOWN);
		break;
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

	// If the user clicked the mouse, they must have clicked within a pane,
	// otherwise we ignore the click. We only send the keypress to that
	// pane and skip the usual logic used for real keypresses.
	if (key == KEY_MOUSE) {
		HandleMouseEvent();
		return true;
	}

	mouse_click_continuation = NULL;

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

bool UI_GetMousePosition(struct pane *if_pane, int *x, int *y)
{
	if (!has_mouse() || mouse_cur_pane == NULL
	 || if_pane != mouse_cur_pane) {
		*x = -1;
		*y = -1;
		return false;
	}
	*x = mouse_cur_x;
	*y = mouse_cur_y;
	return true;
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
	mousemask(BUTTON1_PRESSED|BUTTON4_PRESSED|BUTTON5_PRESSED, NULL);

	actions_bar = UI_ActionsBarInit();
	title_bar = UI_TitleBarInit();
}
