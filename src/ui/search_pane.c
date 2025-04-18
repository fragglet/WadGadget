
#include "ui/search_pane.h"

#include <curses.h>

#include "common.h"
#include "ui/colors.h"
#include "ui/pane.h"
#include "ui/text_input.h"
#include "ui/ui.h"

static bool DrawSearchPane(void *pane)
{
	struct search_pane *p = pane;
	WINDOW *win = p->pane.window;
	int w = getmaxx(win);

	if (getmaxy(win) > 1) {
		wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
		werase(win);
		UI_DrawWindowBox(win);
		mvwaddstr(win, 0, 2, " Search ");
		mvderwin(p->input.win, 1, 2);
		wresize(p->input.win, 1, w - 4);
		if (strlen(p->input.input) > 0) {
			mvwaddstr(win, 0, w - 13, "[   - Next]");
			wattron(win, A_BOLD);
			mvwaddstr(win, 0, w - 12, "^N");
			wattroff(win, A_BOLD);
		}
	} else {
		wbkgdset(win, COLOR_PAIR(PAIR_WHITE_BLACK));
		werase(win);
		mvwaddstr(win, 0, 0, " Search: ");
		mvderwin(p->input.win, 0, 9);
		wresize(p->input.win, 1, w - 9);
	}
	UI_TextInputDraw(&p->input);

	return true;
}

static bool SearchPaneKeypress(void *pane, int key)
{
	struct search_pane *p = pane;

	// Space key triggers mark, does not go to search input.
	if (key != ' ' && UI_TextInputKeypress(&p->input, key)) {
		if (key != KEY_BACKSPACE) {
			// TODO B_DirectoryPaneSearch(active_pane, p->input.input);
		}
	} else {
		// TODO HandleKeypress(NULL, key);
	}

	return true;
}

void UI_InitSearchPane(struct search_pane *sp, WINDOW *win)
{
	assert(win != NULL);
	sp->pane.window = win;
	sp->pane.draw = DrawSearchPane;
	sp->pane.keypress = SearchPaneKeypress;
	sp->pane.mouse_click = NULL;
	UI_TextInputInit(&sp->input, win, 256);
}
