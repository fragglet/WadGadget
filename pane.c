#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "pane.h"

#define MAX_SCREEN_PANES 10

static struct pane *screen_panes[MAX_SCREEN_PANES];
static unsigned int num_screen_panes = 0;
static int main_loop_exited = 0;

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
	static WINDOW *fullscr_win = NULL;
	int i;

	// We maintain a background full-screen window that we just erase
	// entirely before we draw the others. This ensures that any "crud"
	// left over after a window is closed will get erased.
	if (fullscr_win == NULL) {
		fullscr_win = newwin(0, 0, 0, 0);
	}
	werase(fullscr_win);
	wnoutrefresh(fullscr_win);

	for (i = 0; i < num_screen_panes; i++) {
		struct pane *p = screen_panes[i];
		if (p->draw != NULL) {
			p->draw(p);
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

static void HandleKeypresses(void)
{
	int key, i;

	// TODO: This should handle multiple keypresses before returning.

	key = getch();

	// Keypress goes to the top pane that has a keypress handler.
	for (i = num_screen_panes - 1; i >= 0; i--) {
		if (screen_panes[i]->keypress != NULL) {
			UI_PaneKeypress(screen_panes[i], key);
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

