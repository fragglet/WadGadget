#include <curses.h>

#include "pane.h"
#include "ui.h"

static void DrawHeaderPane(void *p, int active)
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

static void DrawInfoPane(void *p, int active)
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

static void DrawSearchPane(void *p, int active)
{
	struct pane *pane = p;

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	box(pane->window, 0, 0);
	mvwaddstr(pane->window, 0, 2, " Search ");
	mvwaddstr(pane->window, 1, 1, "");
}

void UI_InitSearchPane(struct pane *pane, WINDOW *win)
{
	pane->window = win;
	pane->draw = DrawSearchPane;
	pane->keypress = NULL;
}

