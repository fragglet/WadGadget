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

#include "colors.h"
#include "common.h"
#include "pane.h"
#include "ui.h"

#define MAX_NOTICE_LEN    100
#define NOTICE_TIME_SECS    2

static char notice_buf[MAX_NOTICE_LEN + 1];
static time_t last_notice_time;

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
	"[Amulets & Armor", "[Duke Nukem 3D", "[Tank Wars",
	"[Birthright: The Gorgon's Alliance", "[Doom 2D", NULL,
};

#define START_STR "= WadGadget for "
#define END_STR "\b\b and the rest ="

static void DrawHeaderPane(void *p)
{
	struct pane *pane = p;
	int w, h, x, y;
	int i, count_extra;

	if (time(NULL) - last_notice_time < NOTICE_TIME_SECS) {
		wbkgdset(pane->window, COLOR_PAIR(PAIR_HEADER));
		werase(pane->window);
		mvwaddstr(pane->window, 0, 1, notice_buf);
		return;
	}

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

void UI_ShowNotice(const char *msg, ...)
{
	va_list args;

	va_start(args, msg);
	vsnprintf(notice_buf, sizeof(notice_buf), msg, args);
	va_end(args);

	last_notice_time = time(NULL);
}

// These are clock hand directions. Imagine all the border characters as a
// combination of these four directions.
#define DIR_12_OCLOCK   0x1
#define DIR_3_OCLOCK    0x2
#define DIR_6_OCLOCK    0x4
#define DIR_9_OCLOCK    0x8

static int border_chars[16];

// The ACS_ constants are not actually constants, so this needs to be done
// programatically. Sigh.
static void InitBorderChars(void)
{
	border_chars[0] = ' ';
	border_chars[DIR_12_OCLOCK] = 0;
	border_chars[DIR_3_OCLOCK] = 0;
	border_chars[DIR_3_OCLOCK|DIR_12_OCLOCK] = ACS_LLCORNER;
	border_chars[DIR_6_OCLOCK] = 0;
	border_chars[DIR_6_OCLOCK|DIR_12_OCLOCK] = ACS_VLINE;
	border_chars[DIR_6_OCLOCK|DIR_3_OCLOCK] = ACS_ULCORNER;
	border_chars[DIR_6_OCLOCK|DIR_3_OCLOCK|DIR_12_OCLOCK] = ACS_LTEE;
	border_chars[DIR_9_OCLOCK] = 0;
	border_chars[DIR_9_OCLOCK|DIR_12_OCLOCK] = ACS_LRCORNER;
	border_chars[DIR_9_OCLOCK|DIR_3_OCLOCK] = ACS_HLINE;
	border_chars[DIR_9_OCLOCK|DIR_3_OCLOCK|DIR_12_OCLOCK] = ACS_BTEE;
	border_chars[DIR_9_OCLOCK|DIR_6_OCLOCK] = ACS_URCORNER;
	border_chars[DIR_9_OCLOCK|DIR_6_OCLOCK|DIR_12_OCLOCK] = ACS_RTEE;
	border_chars[DIR_9_OCLOCK|DIR_6_OCLOCK|DIR_3_OCLOCK] = ACS_TTEE;
	border_chars[DIR_9_OCLOCK|DIR_6_OCLOCK|DIR_3_OCLOCK|DIR_12_OCLOCK]
		= ACS_PLUS;
}

// Reverse lookup the border bitmask for the given character.
static int BordersForChar(int c)
{
	const int bits = A_CHARTEXT | A_ALTCHARSET;
	int i;

	for (i = 0; i < arrlen(border_chars); i++) {
		if (border_chars[i] != 0
		 && (border_chars[i] & bits) == (c & bits)) {
			return i;
		}
	}

	return -1;
}

static void DrawBorderChar(WINDOW *win, int x, int y, int borders)
{
	int winx, winy, c;
	int curr_borders, new_borders;

	// To read the current character, we have to read from `newscr`
	// (the screen buffer with all changes not yet sent). So we have
	// to calculate the absolute screen location to read from.
	getbegyx(win, winy, winx);
	c = mvwinch(newscr, y + winy, x + winx);

	curr_borders = BordersForChar(c);

	if (curr_borders < 0) {
		// Not a character we understand; we will just overwrite.
		curr_borders = 0;
	}

	new_borders = curr_borders | borders;
	mvwaddch(win, y, x, border_chars[new_borders]);
}

static void DrawHLine(WINDOW *win, int x, int y, int w)
{
	int i;

	for (i = 0; i < w; i++) {
		DrawBorderChar(win, x + i, y, DIR_3_OCLOCK|DIR_9_OCLOCK);
	}
}

static void DrawVLine(WINDOW *win, int x, int y, int h)
{
	int i;

	for (i = 0; i < h; i++) {
		DrawBorderChar(win, x, y + i, DIR_12_OCLOCK|DIR_6_OCLOCK);
	}
}

void UI_DrawBox(WINDOW *win, int x, int y, int w, int h)
{
	static bool initted = false;
	int rx = x + w - 1, by = y + h - 1;

	if (!initted) {
		InitBorderChars();
		initted = true;
	}

	if (w < 2 || h < 2) {
		return;
	}

	DrawBorderChar(win, x, y, DIR_3_OCLOCK|DIR_6_OCLOCK);     // TL
	DrawHLine(win, x + 1, y, w - 2);                          // T
	DrawBorderChar(win, rx, y, DIR_6_OCLOCK|DIR_9_OCLOCK);    // TR
	DrawVLine(win, rx, y + 1, h - 2);                         // R
	DrawBorderChar(win, rx, by, DIR_12_OCLOCK|DIR_9_OCLOCK);  // BR
	DrawVLine(win, x, y + 1, h - 2);                          // B
	DrawBorderChar(win, x, by, DIR_12_OCLOCK|DIR_3_OCLOCK);   // BL
	DrawHLine(win, x + 1, by, w - 2);                         // L
}

void UI_DrawWindowBox(WINDOW *win)
{
	int w, h;

	getmaxyx(win, h, w);
	UI_DrawBox(win, 0, 0, w, h);
}
