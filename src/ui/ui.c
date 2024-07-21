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
#include "ui/ui.h"

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

void UI_DimScreenArea(int x, int y, int w, int h, int force_color_pair)
{
	int x1, y1, color;

	for (y1 = y; y1 < y + h; y1++) {
		for (x1 = x; x1 < x + w; ++x1) {
			chtype c = mvwinch(newscr, y1, x1);
			c = (c | A_DIM) & ~A_BOLD;
			if (force_color_pair < 0) {
				color = PAIR_NUMBER(c);
			} else {
				color = force_color_pair;
			}
			mvwchgat(newscr, y1, x1, 1, c, color, NULL);
		}
	}
}

void UI_DrawDropShadow(WINDOW *win)
{
	int x, y, w, h;

	getbegyx(win, y, x);
	getmaxyx(win, h, w);

	UI_DimScreenArea(x + w, y + 1, 2, h, PAIR_WHITE_BLACK);
	UI_DimScreenArea(x + 2, y + h, w - 2, 1, PAIR_WHITE_BLACK);
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
