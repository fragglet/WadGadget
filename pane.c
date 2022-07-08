#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pane.h"

#define MAX_SCREEN_PANES 10

static struct pane *screen_panes[MAX_SCREEN_PANES];
static unsigned int num_screen_panes = 0;

void UI_PaneKeypress(void *pane, int key)
{
	struct pane *p = pane;

	if (p->keypress != NULL) {
		p->keypress(p, key);
	}
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
	int i;
	struct pane *active_pane = NULL;

	// Active pane is the top pane on the stack that is not an invisible
	// pane (allows for the fake pane case that eats keypresses but does
	// not show anything).
	for (i = num_screen_panes - 1; i >= 0; i--) {
		struct pane *p = screen_panes[i];
		if (p->window != NULL) {
			active_pane = p;
			break;
		}
	}

	for (i = 0; i < num_screen_panes; i++) {
		struct pane *p = screen_panes[i];
		if (p->window != NULL && p->draw != NULL) {
			p->draw(p, p == active_pane);
			wnoutrefresh(p->window);
		}
	}
	doupdate();
}

void UI_RaisePaneToTop(void *pane)
{
	if (UI_PaneHide(pane)) {
		UI_PaneShow(pane);
	}
}

