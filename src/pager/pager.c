//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "pager/pager.h"
#include "ui/actions_bar.h"
#include "ui/dialog.h"
#include "ui/colors.h"
#include "ui/title_bar.h"

struct pager *current_pager;

const struct action exit_pager_action = {
	27, 0, "Close", "Close", UI_ExitMainLoop,
};

static bool LineContainsString(struct pager *p, unsigned int lineno,
                               const char *needle)
{
	int i, j, w, needle_len;

	werase(p->search_pad);
	wmove(p->search_pad, 0, 0);
	p->cfg->draw_line(p->search_pad, lineno, p->cfg->user_data);

	w = getmaxx(p->search_pad);
	needle_len = strlen(needle);
	for (i = 0; i < w - needle_len; i++) {
		for (j = 0; needle[j] != '\0'; j++) {
			int c = mvwinch(p->search_pad, 0, i + j);
			if (tolower(needle[j]) != tolower(c & A_CHARTEXT)) {
				break;
			}
		}
		if (needle[j] == '\0') {
			return true;
		}
	}

	return false;
}

static bool Search(struct pager *p, const char *needle, int start_line)
{
	int i;

	for (i = start_line; i < p->cfg->num_lines; i++) {
		if (LineContainsString(p, i, needle)) {
			p->search_line = i;
			P_JumpWithinWindow(p, i);
			return true;
		}
	}

	// Return to top.
	UI_ShowNotice("Searched to the end; returning to the start.");
	for (i = 0; i < start_line; i++) {
		if (LineContainsString(p, i, needle)) {
			p->search_line = i;
			P_JumpWithinWindow(p, i);
			return true;
		}
	}

	return false;
}

static void PerformSearch(void)
{
	char *needle;

	needle = UI_TextInputDialogBox(
		"Search", "Search", 32, "Enter search string:");

	if (needle == NULL) {
		return;
	}

	if (!Search(current_pager, needle, current_pager->window_offset)) {
		UI_ShowNotice("No match found.");
		return;
	}

	free(current_pager->last_search);
	current_pager->last_search = checked_strdup(needle);
}

const struct action pager_search_action = {
	0, 'F', "Search", "Search", PerformSearch,
};

static void PerformSearchAgain(void)
{
	int last_search_line;

	if (current_pager->last_search == NULL) {
		PerformSearch();
		return;
	}

	last_search_line = current_pager->search_line;

	Search(current_pager, current_pager->last_search,
	       current_pager->search_line + 1);
	if (current_pager->search_line == last_search_line) {
		UI_ShowNotice("No more matches found.");
	}
}

const struct action pager_search_again_action = {
	0, 'N', "Next", "Search Again", PerformSearchAgain,
};

static void UpdateSubtitle(struct pager *p)
{
	int range, win_h;

	win_h = getmaxy(p->pane.window);
	range = p->cfg->num_lines > win_h ?
	        p->cfg->num_lines - win_h : 0;
	p->window_offset = min(p->window_offset, range);
	if (range > 0) {
		snprintf(p->subtitle, sizeof(p->subtitle), "%d%%",
		         min(100, p->window_offset * 100 / range));
		UI_SetSubtitle(p->subtitle);
	} else {
		UI_SetSubtitle(NULL);
	}
}

static bool DrawPager(void *_p)
{
	struct pager *p = _p;
	int y, curs_y, lineno, win_h;

	assert(wresize(p->pane.window, LINES - 2, COLS) == OK);
	assert(wresize(p->line_win, 1, COLS) == OK);

	UpdateSubtitle(p);

	wbkgdset(p->line_win, COLOR_PAIR(PAIR_WHITE_BLACK));
	werase(p->pane.window);

	lineno = p->window_offset;
	win_h = getmaxy(p->pane.window);
	for (y = 0; y < win_h && lineno < p->cfg->num_lines; ++y, ++lineno) {
		assert(mvderwin(p->line_win, y, 0) == OK);
		if (lineno == p->search_line) {
			wbkgdset(p->line_win, COLOR_PAIR(PAIR_NOTICE));
		} else {
			wbkgdset(p->line_win, COLOR_PAIR(PAIR_WHITE_BLACK));
		}
		werase(p->line_win);
		mvwaddstr(p->line_win, 0, 0, "");
		p->cfg->draw_line(p->line_win, lineno, p->cfg->user_data);
	}

	if (p->cfg->num_lines > win_h) {
		curs_y = (p->window_offset * (win_h - 1))
		       / (p->cfg->num_lines - win_h);
	} else {
		curs_y = win_h - 1;
	}
	mvwaddstr(p->pane.window, curs_y, getmaxx(p->pane.window) - 1, "");

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
	p->search_pad = newpad(1, 120);
	p->search_line = -1;
	p->last_search = NULL;
	p->cfg = cfg;
}

void P_FreePager(struct pager *p)
{
	delwin(p->line_win);
	delwin(p->pane.window);
	delwin(p->search_pad);
	free(p->last_search);
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
	P_ClearSearch(current_pager);
	UI_ActionsBarSetActions(cfg->actions);
}

void P_JumpToLine(struct pager *p, int lineno)
{
	int win_h;

	if (p->pane.window == NULL) {
		p->window_offset = lineno;
		return;
	}

	win_h = getmaxy(p->pane.window);
	p->window_offset = max(min(lineno, p->cfg->num_lines - win_h), 0);
}

// P_JumpToLine, but keep current window position unless the line is
// not visible.
void P_JumpWithinWindow(struct pager *p, int lineno)
{
	int win_h;

	if (p->pane.window == NULL) {
		win_h = 25;
	} else {
		win_h = getmaxy(p->pane.window);
	}

	if (lineno < p->window_offset || lineno >= p->window_offset + win_h) {
		P_JumpToLine(current_pager, lineno - 5);
	}
}

void P_ClearSearch(struct pager *p)
{
	current_pager->search_line = -1;
}
