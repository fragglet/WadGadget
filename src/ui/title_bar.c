//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <time.h>

#include "ui/colors.h"
#include "common.h"
#include "ui/pane.h"
#include "ui/stack.h"
#include "ui/title_bar.h"
#include "ui/ui.h"

#define MAX_NOTICE_LEN    100
#define NOTICE_TIME_SECS    2

static char notice_buf[MAX_NOTICE_LEN + 1];
static time_t last_notice_time;

// What games use the WAD format?
static const char *games[] = {
	"Doom", "[Doom II", "[Final Doom", "Heretic", "Hexen", "Strife",
	"[Chex Quest", "[Freedoom", "[Rise of the Triad", "[HACX",
	"[Amulets & Armor", "[Duke Nukem 3D", "[Tank Wars",
	"[Birthright: The Gorgon's Alliance", "[Doom 2D", NULL,
};

#define START_STR "= WadGadget for "
#define END_STR "\b\b and the rest ="

struct title_bar {
	struct pane pane;
};

static struct title_bar title_bar_singleton;

static void DrawCutesyTitle(WINDOW *win, int w)
{
	int x;
	int i, count_extra = 0;

	mvwaddstr(win, 0, 1, START_STR);

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
			waddstr(win, games[i]);
			waddstr(win, ", ");
		} else if (count_extra > 0) {
			waddstr(win, games[i] + 1);
			waddstr(win, ", ");
			--count_extra;
		}
		x = getcurx(win);
	}

	waddstr(win, END_STR);
}

static bool DrawTitleBar(void *_p)
{
	struct title_bar *p = _p;
	const struct pane_stack *stack = UI_CurrentStack();
	const char *title = stack->title, *subtitle = stack->subtitle;

	mvwin(p->pane.window, stack->state.top_line, 0);

	if (time(NULL) - last_notice_time < NOTICE_TIME_SECS) {
		wbkgdset(p->pane.window, COLOR_PAIR(PAIR_NOTICE));
		werase(p->pane.window);
		mvwaddstr(p->pane.window, 0, 1, notice_buf);
		return true;
	}

	wbkgdset(p->pane.window, COLOR_PAIR(PAIR_HEADER));
	werase(p->pane.window);
	wattron(p->pane.window, A_BOLD);

	if (title != NULL) {
		waddstr(p->pane.window, " ");
		waddstr(p->pane.window, title);
	} else {
		int w = getmaxx(p->pane.window);
		w -= subtitle != NULL ? strlen(subtitle) + 3 : 0;

		DrawCutesyTitle(p->pane.window, w);
	}
	if (subtitle != NULL) {
		int x = getmaxx(p->pane.window) - strlen(subtitle) - 3;
		mvwaddstr(p->pane.window, 0, x, " ");
		waddstr(p->pane.window, subtitle);
	}
	wattroff(p->pane.window, A_BOLD);

	return true;
}

struct pane *UI_TitleBarInit(void)
{
	memset(&title_bar_singleton, 0, sizeof(struct title_bar));

	title_bar_singleton.pane.window = newwin(1, COLS, 0, 0);
	title_bar_singleton.pane.draw = DrawTitleBar;
	title_bar_singleton.pane.keypress = NULL;

	return &title_bar_singleton.pane;
}

void UI_ShowNotice(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vsnprintf(notice_buf, sizeof(notice_buf), msg, args);
	va_end(args);

	last_notice_time = time(NULL);
}
