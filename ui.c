#include <curses.h>

#include "pane.h"
#include "ui.h"

static void DrawHeader(void *p)
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
	pane->draw = DrawHeader;
	pane->keypress = NULL;
}

