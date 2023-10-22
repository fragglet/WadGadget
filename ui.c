#include <stdlib.h>
#include <string.h>
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

// What games use the WAD format?
static const char *games[] = {
	"Doom", "[Doom II", "[Final Doom", "Heretic", "Hexen", "Strife",
	"[Chex Quest", "[Freedoom", "[Rise of the Triad", "[HACX",
	"[Amulets & Armor", "[Duke Nukem 3D", NULL,
};

#define START_STR "= WadGadget for "
#define END_STR "\b\b and the rest ="

static void DrawHeaderPane(void *p)
{
	struct pane *pane = p;
	int w, h, x, y;
	int i, count_extra;

	getmaxyx(pane->window, h, w);
	h = h;

	wbkgdset(pane->window, COLOR_PAIR(PAIR_HEADER));
	werase(pane->window);
	wattron(pane->window, A_BOLD);
	mvwaddstr(pane->window, 0, 1, START_STR);

	count_extra = 0;
	x = strlen(START_STR) + strlen(END_STR);
	for (i = 0; games[i] != NULL && x + strlen(games[i]) < w; i++) {
		if (games[i][0] != '[') {
			x += strlen(games[i]) + 2;
		}
	}
	for (i = 0; games[i] != NULL && x + strlen(games[i]) < w; i++) {
		if (games[i][0] == '[') {
			x += strlen(games[i]) + 1;
			++count_extra;
		}
	}
	x = 0;
	for (i = 0; games[i] != NULL; i++) {
		if (x + strlen(games[i]) > w - strlen(END_STR)) {
			continue;
		}
		if (games[i][0] != '[') {
			waddstr(pane->window, games[i]);
			waddstr(pane->window, ", ");
		} else if (count_extra > 0) {
			waddstr(pane->window, games[i] + 1);
			waddstr(pane->window, ", ");
			--count_extra;
		}
		getyx(pane->window, y, x);
		y = y;
	}

	waddstr(pane->window, END_STR);
	wattroff(pane->window, A_BOLD);
}

void UI_InitHeaderPane(struct pane *pane, WINDOW *win)
{
	pane->window = win;
	pane->draw = DrawHeaderPane;
	pane->keypress = NULL;
}

