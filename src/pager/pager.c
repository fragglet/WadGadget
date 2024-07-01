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
#include "ui/actions_bar.h"
#include "ui/colors.h"
#include "ui/title_bar.h"

struct pager *current_pager;

static void UpdateSubtitle(struct pager *p)
{
	int range, win_h;

	win_h = getmaxy(p->pane.window);
	range = p->cfg->num_lines > win_h ?
	        p->cfg->num_lines - win_h + 1: 0;
	p->window_offset = min(p->window_offset, range);
	if (range > 0) {
		snprintf(p->subtitle, sizeof(p->subtitle), "%d%%",
		         min(100, p->window_offset * 100 / range));
	}

	UI_SetSubtitle(p->subtitle);
}

static bool DrawPager(void *_p)
{
	struct pager *p = _p;
	int y, lineno, win_h;

	assert(wresize(p->pane.window, LINES - 2, COLS) == OK);
	assert(wresize(p->line_win, 1, COLS) == OK);

	UpdateSubtitle(p);

	wbkgdset(p->line_win, COLOR_PAIR(PAIR_WHITE_BLACK));

	lineno = p->window_offset;
	win_h = getmaxy(p->pane.window);
	for (y = 0; y < win_h && lineno < p->cfg->num_lines; ++y, ++lineno) {
		assert(mvderwin(p->line_win, y, 0) == OK);
		werase(p->line_win);
		mvwaddstr(p->line_win, 0, 0, "");
		p->cfg->draw_line(p->line_win, lineno, p->cfg->user_data);
	}

	mvwaddstr(p->pane.window, win_h - 1, getmaxx(p->pane.window) - 1, "");

	return true;
}

static void HandleKeypress(void *_p, int c)
{
	struct pager *p = _p;
	int i;
	int win_h = getmaxy(p->pane.window);

	switch (c) {
	case 'q':
	case 'Q':
	case 27:
		UI_ExitMainLoop();
		break;
	case KEY_UP:
		if (p->window_offset > 0) {
			--p->window_offset;
		}
		break;
	case KEY_DOWN:
		if (p->window_offset + win_h < p->cfg->num_lines) {
			++p->window_offset;
		}
		break;
	case KEY_PPAGE:
		for (i = 0; i < win_h; i++) {
			HandleKeypress(p, KEY_UP);
		}
		break;
	case KEY_NPAGE:
		for (i = 0; i < win_h; i++) {
			HandleKeypress(p, KEY_DOWN);
		}
		break;
	case KEY_HOME:
		p->window_offset = 0;
		break;
	case KEY_END:
		p->window_offset = p->cfg->num_lines < win_h ? 0 :
		                   p->cfg->num_lines - win_h;
		break;
	case KEY_RESIZE:
		refresh();
		break;
	}
}

void P_InitPager(struct pager *p, struct pager_config *cfg)
{
	memset(p, 0, sizeof(struct pager));

	p->pane.window = newwin(LINES - 2, COLS, 1, 0);
	p->pane.keypress = HandleKeypress;
	p->pane.draw = DrawPager;
	p->line_win = derwin(p->pane.window, 1, COLS, 0, 0);
	p->cfg = cfg;
}

void P_FreePager(struct pager *p)
{
	delwin(p->line_win);
	delwin(p->pane.window);
}

void P_RunPager(struct pager_config *cfg)
{
	struct saved_screen ss;
	struct pager p;

	current_pager = &p;

	UI_SaveScreen(&ss);
	UI_SetTitleBar(cfg->title);
	UI_ActionsBarSetActions(cfg->actions);
	P_InitPager(&p, cfg);
	UI_PaneShow(&p);
	UI_RunMainLoop();
	UI_PaneHide(&p);
	P_FreePager(&p);
	UI_RestoreScreen(&ss);

	current_pager = NULL;
}

void P_SwitchConfig(struct pager_config *cfg)
{
	assert(current_pager != NULL);
	current_pager->cfg = cfg;
	current_pager->window_offset = 0;
	UI_ActionsBarSetActions(cfg->actions);
}
