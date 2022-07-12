#include <stdlib.h>
#include <curses.h>

#include "colors.h"
#include "pane.h"
#include "ui.h"

static void DrawHeaderPane(void *p)
{
	struct pane *pane = p;

	wbkgdset(pane->window, COLOR_PAIR(PAIR_HIGHLIGHT));
	werase(pane->window);
	mvwaddstr(pane->window, 0, 1, "= WadGadget for Doom, Heretic, Hexen, "
	                "Strife, Chex Quest and the rest =");
}

void UI_InitHeaderPane(struct pane *pane, WINDOW *win)
{
	pane->window = win;
	pane->draw = DrawHeaderPane;
	pane->keypress = NULL;
}

static void DrawInfoPane(void *p)
{
	struct pane *pane = p;

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	box(pane->window, 0, 0);
	mvwaddstr(pane->window, 0, 2, " Info ");

	mvwaddstr(pane->window, 1, 2, "TITLEPIC  123 bytes");
	mvwaddstr(pane->window, 2, 2, "Dimensions: 320x200");
}

void UI_InitInfoPane(struct pane *pane, WINDOW *win)
{
	pane->window = win;
	pane->draw = DrawInfoPane;
	pane->keypress = NULL;
}

static void DrawSearchPane(void *pane)
{
	struct search_pane *p = pane;
	WINDOW *win = p->pane.window;

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 0, 2, " Search ");
	UI_TextInputDraw(&p->input);
}

static void SearchPaneKeypress(void *pane, int key)
{
	struct search_pane *p = pane;
	UI_TextInputKeypress(&p->input, key);
}

void UI_InitSearchPane(struct search_pane *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawSearchPane;
	pane->pane.keypress = SearchPaneKeypress;
	UI_TextInputInit(&pane->input, win, 1, 20);
}

