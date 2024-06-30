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
#include <assert.h>

#include "ui/actions_bar.h"
#include "ui/colors.h"
#include "ui/pane.h"
#include "ui/title_bar.h"

#define MAX_SCREEN_PANES 10

static struct pane *actions_bar, *title_bar;
static struct pane *bottom_pane;
static int main_loop_exited = 0;

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

	ptr = &bottom_pane;
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

void UI_DrawAllPanes(void)
{
	struct pane *p;

	wbkgdset(newscr, COLOR_PAIR(PAIR_WHITE_BLACK));
	werase(newscr);

	for (p = bottom_pane; p != NULL; p = p->next) {
		UI_DrawPane(p);
	}
	UI_DrawPane(actions_bar);

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
	struct pane *p = bottom_pane;

	while (p != NULL) {
		if (p->next == pane) {
			return p;
		}
		p = p->next;
	}

	return NULL;
}

static bool HandleKeypress(void)
{
	struct pane *p;
	int key;

	// TODO: This should handle multiple keypresses before returning.
	key = getch();
	if (key == ERR) {
		return false;
	}

	// Keypress goes to the top pane that has a keypress handler.
	for (p = GetPrevPane(NULL); p != NULL; p = GetPrevPane(p)) {
		if (p->keypress != NULL) {
			UI_PaneKeypress(p, key);
			break;
		}
	}

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

	main_loop_exited = 0;
}

void UI_ExitMainLoop(void)
{
	main_loop_exited = 1;
}

void UI_Init(void)
{
	actions_bar = UI_ActionsBarInit();
	title_bar = UI_TitleBarInit();
	UI_PaneShow(title_bar);
}
