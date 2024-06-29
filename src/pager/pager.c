//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <string.h>

#include "common.h"
#include "pager/pager.h"
#include "ui/colors.h"

static void DrawPager(void *_p)
{
	struct pager *p = _p;
	char buf[10];
	int y, lineno, range;

	werase(newscr);
	wresize(p->pane.window, LINES, COLS);
	wresize(p->line_win, 1, COLS);

	// Draw the top title row.
	// TODO: This should not be part of the pager itself.
	mvwin(p->line_win, 0, 0);
	wbkgd(p->line_win, COLOR_PAIR(PAIR_HEADER));
	werase(p->line_win);
	wattron(p->line_win, A_BOLD);
	mvwaddstr(p->line_win, 0, 0, " ");
	if (p->cfg->title != NULL) {
		waddstr(p->line_win, p->cfg->title);
	}

	range = p->cfg->num_lines > (LINES - 1) ?
	        p->cfg->num_lines - LINES + 1: 0;
	p->window_offset = min(p->window_offset, range);
	if (range > 0) {
		snprintf(buf, sizeof(buf), "%d%%",
		         min(100, p->window_offset * 100 / range));
		mvwaddstr(p->line_win, 0, COLS - strlen(buf) - 2, buf);
	}
	wattroff(p->line_win, A_BOLD);
	wnoutrefresh(p->line_win);

	wbkgdset(p->line_win, COLOR_PAIR(PAIR_WHITE_BLACK));

	lineno = p->window_offset;
	for (y = 1; y < LINES && lineno < p->cfg->num_lines; ++y, ++lineno) {
		mvwin(p->line_win, y, 0);
		werase(p->line_win);
		p->cfg->draw_line(p->line_win, lineno, p->cfg->user_data);
		wnoutrefresh(p->line_win);
	}

	mvaddstr(LINES - 1, COLS - 1, "");

	doupdate();
}

void P_InitPager(struct pager *p, struct pager_config *cfg)
{
	memset(p, 0, sizeof(struct pager));

	p->pane.window = newwin(1, COLS, 0, 0);
	p->pane.draw = DrawPager;
	p->line_win = subwin(p->pane.window, 1, COLS, 0, 0);
	p->cfg = cfg;
}

void P_FreePager(struct pager *p)
{
	delwin(p->line_win);
	delwin(p->pane.window);
}

static void HandleKeypress(struct pager *p, int c)
{
	int i;

	switch (c) {
	case 'q':
	case 'Q':
	case 27:
		p->done = true;
		break;
	case KEY_UP:
		if (p->window_offset > 0) {
			--p->window_offset;
		}
		break;
	case KEY_DOWN:
		if (p->window_offset + LINES - 1 < p->cfg->num_lines) {
			++p->window_offset;
		}
		break;
	case KEY_PPAGE:
		for (i = 0; i < LINES - 1; i++) {
			HandleKeypress(p, KEY_UP);
		}
		break;
	case KEY_NPAGE:
		for (i = 0; i < LINES - 1; i++) {
			HandleKeypress(p, KEY_DOWN);
		}
		break;
	case KEY_HOME:
		p->window_offset = 0;
		break;
	case KEY_END:
		p->window_offset = p->cfg->num_lines < LINES - 1 ? 0 :
		                   p->cfg->num_lines - LINES + 1;
		break;
	case KEY_RESIZE:
		refresh();
		break;
	}
}

void P_BlockOnInput(struct pager *p)
{
	int c = getch();
	HandleKeypress(p, c);
}

void P_RunPager(struct pager_config *cfg)
{
	struct pager p;

	P_InitPager(&p, cfg);
	while (!p.done) {
		DrawPager(&p);
		P_BlockOnInput(&p);
	}
	P_FreePager(&p);
}
