#include "pane.h"

void UI_PaneDraw(void *pane)
{
	struct pane *p = pane;

	if (p->draw != NULL) {
		p->draw(pane);
	}
}

void UI_PaneKeypress(void *pane, int key)
{
	struct pane *p = pane;

	if (p->keypress != NULL) {
		p->keypress(p, key);
	}
}

void UI_PaneActive(void *pane, int active)
{
	struct pane *p = pane;

	p->active = active;
}

