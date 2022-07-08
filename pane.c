#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pane.h"

#define MAX_SCREEN_PANES 10

static struct pane *screen_panes[MAX_SCREEN_PANES];
static unsigned int num_screen_panes = 0;

void UI_PaneDraw(void *pane)
{
	struct pane *p = pane;

	if (p->draw != NULL) {
		p->draw(pane);
		wnoutrefresh(p->window);
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

void UI_PaneShow(void *pane)
{
	assert(num_screen_panes < MAX_SCREEN_PANES);

	screen_panes[num_screen_panes] = pane;
	++num_screen_panes;
}

int UI_PaneHide(void *pane)
{
	struct pane *p = pane;
	unsigned int i;

	for (i = 0; i < num_screen_panes; i++) {
		if (screen_panes[i] == p) {
			memmove(&screen_panes[i], &screen_panes[i+1],
			        (num_screen_panes - i - 1)
			            * sizeof(struct pane *));
			--num_screen_panes;
			return 1;
		}
	}

	return 0;
}

void UI_DrawAllPanes(void)
{
	unsigned int i;

	for (i = 0; i < num_screen_panes; i++) {
		UI_PaneDraw(screen_panes[i]);
	}
	doupdate();
}

void UI_RaisePaneToTop(void *pane)
{
	if (UI_PaneHide(pane)) {
		UI_PaneShow(pane);
	}
}

