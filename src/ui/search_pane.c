
#include "ui/search_pane.h"

#include <curses.h>

#include "common.h"
#include "ui/colors.h"
#include "ui/pane.h"
#include "ui/text_input.h"
#include "ui/title_bar.h"
#include "ui/ui.h"

static int PrefixSearch(struct search_pane *sp, const char *needle,
                        int start_index)
{
	size_t needle_len = strlen(needle);
	const char *haystack;
	int i;

	for (i = start_index;; i++) {
		haystack = sp->element_text(i, sp->callback_data);
		if (haystack == NULL) {
			break;
		}
		if (!strncasecmp(haystack, needle, needle_len)) {
			return i;
		}
	}

	return -1;
}

static int SubstringSearch(struct search_pane *sp, const char *needle,
                           int start_index)
{
	size_t haystack_len, needle_len = strlen(needle);
	const char *haystack;
	int i, j;

	for (i = start_index;; i++) {
		haystack = sp->element_text(i, sp->callback_data);
		if (haystack == NULL) {
			break;
		}
		haystack_len = strlen(haystack);
		if (haystack_len < needle_len) {
			continue;
		}
		for (j = 0; j < haystack_len - needle_len + 1; j++) {
			if (!strncasecmp(haystack + j, needle, needle_len)) {
				return i;
			}
		}
	}

	return -1;
}

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

static void PerformSearch(struct search_pane *sp, const char *needle)
{
	int idx;

	if (strlen(needle) == 0) {
		return;
	}

	// Check for prefix first, so user can type entire lump name.
	idx = PrefixSearch(sp, needle, 0);

	// If nothing found, try a substring match.
	if (idx < 0) {
		idx = SubstringSearch(sp, needle, 0);
	}

	if (idx >= 0) {
		sp->search_found(idx, sp->callback_data);
	}
}

void UI_SearchAgain(struct search_pane *sp, unsigned int start_index)
{
	const char *needle = sp->input.input;
	int idx;

	if (strlen(needle) == 0) {
		return;
	}

	// When searching again, we only do substring matches.
	idx = SubstringSearch(sp, needle, start_index + 1);
	if (idx < 0) {
		idx = SubstringSearch(sp, needle, 0);
	}
	if (idx >= 0) {
		sp->search_found(idx, sp->callback_data);
	}

	if (idx < 0) {
		UI_ShowNotice("No matches found.");
	} else if (idx < start_index) {
		UI_ShowNotice("Searched to the end; returning to the start.");
	} else if (idx == start_index) {
		UI_ShowNotice("No other matches found.");
	}
}

static bool SearchPaneKeypress(void *pane, int key)
{
	struct search_pane *sp = pane;

	if (!UI_TextInputKeypress(&sp->input, key)) {
		return false;
	}

	if (key != KEY_BACKSPACE) {
		PerformSearch(sp, sp->input.input);
	}

	return true;
}

void UI_InitSearchPane(struct search_pane *sp, WINDOW *win,
                       element_text_func callback,
                       search_found_func search_found, void *callback_data)
{
	assert(win != NULL);
	sp->pane.window = win;
	sp->pane.draw = DrawSearchPane;
	sp->pane.keypress = SearchPaneKeypress;
	sp->pane.mouse_click = NULL;
	UI_TextInputInit(&sp->input, win, 256);
	sp->element_text = callback;
	sp->search_found = search_found;
	sp->callback_data = callback_data;
}
