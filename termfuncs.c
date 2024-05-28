//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "colors.h"
#include "common.h"
#include "termfuncs.h"

#define CLEAR_SCREEN_ESCAPE "\x1b[H\x1b[2J"
#define RESPONSE_TIMEOUT 1000 /* ms */

// We use the curses init_color() function to set a custom color palette
// that matches the palette from NWT; these values are from the ScreenPal[]
// array in wadview.c.
#define V(x) ((x * 1000) / 63)

static struct palette nwt_palette = {
	16,
	{
		{COLOR_BLACK,          V( 0), V( 0), V( 0)},
		{COLOR_BLUE,           V( 0), V( 0), V(25)},
		{COLOR_GREEN,          V( 0), V(42), V( 0)},
		{COLOR_CYAN,           V( 0), V(42), V(42)},
		{COLOR_RED,            V(42), V( 0), V( 0)},
		{COLOR_MAGENTA,        V(42), V( 0), V(42)},
		{COLOR_YELLOW,         V(42), V(42), V( 0)},
		{COLOR_WHITE,          V(34), V(34), V(34)},

		{COLORX_DARKGREY,      V( 0), V( 0), V(13)},
		{COLORX_BRIGHTBLUE,    V( 0), V( 0), V(55)},
		{COLORX_BRIGHTGREEN,   V( 0), V(34), V(13)},
		{COLORX_BRIGHTCYAN,    V( 0), V(34), V(55)},
		{COLORX_BRIGHTRED,     V(34), V( 0), V(13)},
		{COLORX_BRIGHTMAGENTA, V(34), V( 0), V(55)},
		{COLORX_BRIGHTYELLOW,  V(34), V(34), V(13)},
		{COLORX_BRIGHTWHITE,   V(55), V(55), V(55)},
	},
};

static const struct {
	int pair_index, fg, bg;
} color_pairs[] = {
	{PAIR_WHITE_BLACK, COLORX_BRIGHTWHITE, COLOR_BLACK},
	{PAIR_PANE_COLOR,  COLORX_BRIGHTWHITE, COLOR_BLUE},
	{PAIR_HEADER,      COLOR_BLACK,        COLORX_BRIGHTCYAN},
	{PAIR_DIRECTORY,   COLOR_WHITE,        COLOR_BLACK},
	{PAIR_WAD_FILE,    COLOR_RED,          COLOR_BLACK},
	{PAIR_DIALOG_BOX,  COLORX_BRIGHTWHITE, COLOR_MAGENTA},
	{PAIR_TAGGED,      COLORX_BRIGHTWHITE, COLOR_RED},
	{PAIR_NOTICE,      COLOR_BLACK,        COLOR_YELLOW},
};

// Old palette we saved and restore on quit.
static struct palette old_palette;

void TF_SetCursesModes(void)
{
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	TF_SetPalette(&nwt_palette);
}

void TF_SuspendCursesMode(void)
{
	// Restore the normal palette.
	TF_SetPalette(&old_palette);

	// We clear the screen first to avoid a brief flash of palette
	// switching during the endwin() call.
	clear();
	refresh();

	endwin();
}

void TF_SavePalette(struct palette *p)
{
	short r, g, b;
	int i;

	p->num_colors = 16;
	for (i = 0; i < p->num_colors; i++) {
		p->colors[i].c = i;
		color_content(i, &r, &g, &b);
		p->colors[i].r = r;
		p->colors[i].g = g;
		p->colors[i].b = b;
	}
}

void TF_SetPalette(struct palette *p)
{
	int i;

	if (!has_colors() || !can_change_color()) {
		return;
	}

	for (i = 0; i < p->num_colors; i++) {
		if (p->colors[i].c >= COLORS) {
			continue;
		}
		init_color(p->colors[i].c, p->colors[i].r, p->colors[i].g,
		           p->colors[i].b);
	}
}

void TF_SetColorPairs(void)
{
	int i, mask = 0xff;

	// If we do not support extended colors, we
	// instead fall back to the standard colors.
	if (COLORS < 16) {
		mask = 0x07;
	}

	for (i = 0; i < arrlen(color_pairs); i++) {
		init_pair(color_pairs[i].pair_index,
		          color_pairs[i].fg & mask,
		          color_pairs[i].bg & mask);
	}
}

void TF_SetNewPalette(void)
{
	TF_SavePalette(&old_palette);
	TF_SetPalette(&nwt_palette);
}

void TF_RestoreOldPalette(void)
{
	TF_SetPalette(&old_palette);
}

void TF_SetRawMode(struct saved_flags *f, bool blocking)
{
	struct termios raw;

	// Don't block reads from stdin; we want to time out.
	if (!blocking) {
		f->fcntl_opts = fcntl(0, F_GETFL);
		fcntl(0, F_SETFL, f->fcntl_opts | O_NONBLOCK);
	}

	// Don't print the response, and don't wait for a newline.
	tcgetattr(0, &f->termios);
	raw = f->termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(0, TCSAFLUSH, &raw);
}

void TF_RestoreNormalMode(struct saved_flags *f)
{
	fcntl(0, F_SETFL, f->fcntl_opts);
	tcsetattr(0, TCSAFLUSH, &f->termios);
}

static int TimeDiffMs(struct timeval *a, struct timeval *b)
{
	return (a->tv_sec - b->tv_sec) * 1000
	     + (a->tv_usec - b->tv_usec) / 1000;
}

// Read a character from stdin, timing out if no response is received
// in RESPONSE_TIMEOUT milliseconds.
int TF_PollingReadChar(struct timeval *start)
{
	struct timeval now;
	int c;

	for (;;) {
		gettimeofday(&now, NULL);
		if (TimeDiffMs(&now, start) > RESPONSE_TIMEOUT) {
			errno = EWOULDBLOCK;
			return 0;
		}
		errno = 0;
		c = getchar();
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			usleep(1000);
			continue;
		} else if (errno != 0) {
			return 0;
		}
		return c;
	}
}

void TF_ClearScreen(void)
{
	write(1, CLEAR_SCREEN_ESCAPE, strlen(CLEAR_SCREEN_ESCAPE));
}
