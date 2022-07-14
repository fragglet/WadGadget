#include <stdlib.h>
#include <curses.h>

#include "colors.h"
#include "common.h"
#include "pane.h"
#include "ui.h"

int UI_StringWidth(char *s)
{
	int max_width = 0, cur_width = 0;
	char *p;

	for (p = s; *p != '\0'; ++p) {
		if (*p != '\n') {
			++cur_width;
			continue;
		}
		max_width = max(max_width, cur_width);
		cur_width = 0;
	}
	return max(cur_width, max_width);
}

int UI_StringHeight(char *s)
{
	int lines = 1;
	char *p;

	for (p = s; *p != '\0'; ++p) {
		if (*p == '\n' && *(p+1) != '\0') {
			++lines;
		}
	}
	return lines;
}

// Print string at given position in window, with newlines wrapping to the
// next line at the same starting x position.
void UI_PrintMultilineString(WINDOW *win, int y, int x, const char *s)
{
	const char *p;

	wmove(win, y, x);
	for (p = s; *p != '\0'; ++p) {
		if (*p == '\n') {
			++y;
			wmove(win, y, x);
		} else {
			waddch(win, *p);
		}
	}
}

static void DrawHeaderPane(void *p)
{
	struct pane *pane = p;

	wbkgdset(pane->window, COLOR_PAIR(PAIR_HEADER));
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

