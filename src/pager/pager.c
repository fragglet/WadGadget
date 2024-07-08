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

static void PerformSearchAgain(void);

static void PerformSearch(void)
{
	char *needle;

	needle = UI_TextInputDialogBox(
		"Search", "Search", 32, "Enter search string:");

	if (needle == NULL) {
		current_pager->search_line = -1;
		return;
	}
	// Search again if the user presses /, enter. Because I'm a
	// vim user and it's baked into my muscle memory.
	if (strlen(needle) == 0) {
		free(needle);
		if (current_pager->last_search != NULL) {
			PerformSearchAgain();
		}
		return;
	}

	if (!Search(current_pager, needle, current_pager->window_offset)) {
		UI_ShowNotice("No match found.");
		return;
	}

	free(current_pager->last_search);
	current_pager->last_search = needle;
}

const struct action pager_search_action = {
	'/', 'F', "Search", "Search", PerformSearch,
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
	'n', 'N', "Next", "Search Again", PerformSearchAgain,
};

static void PerformNextLink(void)
{
	struct pager_config *cfg = current_pager->cfg;

	if (cfg->current_link + 1 >= cfg->num_links) {
		return;
	}

	++cfg->current_link;
	if (current_pager != NULL) {
		struct pager_link l;
		cfg->get_link(cfg, cfg->current_link, &l);
		P_JumpWithinWindow(current_pager, l.lineno);
	}
}

const struct action pager_next_link_action = {
        '\t', 0, "NextLink", "Next Link", PerformNextLink,
};

static void PerformPrevLink(void)
{
	struct pager_config *cfg = current_pager->cfg;

	if (cfg->current_link <= 0) {
		return;
	}
	--cfg->current_link;
	if (current_pager != NULL) {
		struct pager_link l;
		cfg->get_link(cfg, cfg->current_link, &l);
		P_JumpWithinWindow(current_pager, l.lineno);
	}
}

const struct action pager_prev_link_action = {
	KEY_BTAB, 0, NULL, NULL, PerformPrevLink,
};

enum line_location { OUTSIDE_WINDOW, INSIDE_WINDOW, ON_WINDOW_EDGE };

static enum line_location LinkWithinWindow(struct pager *p, int link)
{
	int win_h = getmaxy(p->pane.window);
	struct pager_link l;
	int lineno;

	if (link < 0 || link >= p->cfg->num_links) {
		return OUTSIDE_WINDOW;
	}

	p->cfg->get_link(p->cfg, link, &l);
	lineno = l.lineno;

	if (lineno == p->window_offset - 1
	 || lineno == p->window_offset + win_h) {
		return ON_WINDOW_EDGE;
	} else if (lineno >= p->window_offset
	        && lineno < p->window_offset + win_h) {
		return INSIDE_WINDOW;
	} else {
		return OUTSIDE_WINDOW;
	}
}

// Returns index of link with largest index and line number < lineno
static int FindByLineno(struct pager_config *cfg, int lineno)
{
	struct pager_link l;
	int low = 0, high = cfg->num_links - 1, idx;

	if (cfg->num_links == 0) {
		return -1;
	}
	cfg->get_link(cfg, 0, &l);
	if (lineno < l.lineno) {
		return -1;
	}

	while (high - low >= 2) {
		idx = (low + high) / 2;
		cfg->get_link(cfg, idx, &l);
		if (l.lineno < lineno) {
			low = idx;
		} else {
			high = idx;
		}
	}

	return low;
}

static void PagerMoved(struct pager *p)
{
	struct pager_config *cfg = p->cfg;
	struct pager_link l;
	int win_h = getmaxy(p->pane.window);
	int new_link;

	if (cfg->current_link < 0 || cfg->num_links == 0) {
		return;
	}

	if (LinkWithinWindow(p, cfg->current_link) == INSIDE_WINDOW) {
		return;
	}

	// Currently selected link is not within the visible window, so
	// we should try to find a new link.
	cfg->get_link(cfg, cfg->current_link, &l);
	if (l.lineno < p->window_offset) {
		new_link = FindByLineno(cfg, p->window_offset);
		if (new_link >= 0 && new_link < cfg->num_links - 1) {
			++new_link;
		}
	} else {
		new_link = FindByLineno(cfg, p->window_offset + win_h - 1);
	}

	if (LinkWithinWindow(p, new_link) == INSIDE_WINDOW) {
		cfg->current_link = new_link;
	}
}

static void SetWindowOffset(struct pager *p, int lineno)
{
	if (p->window_offset == lineno) {
		return;
	}

	p->window_offset = lineno;
	PagerMoved(p);
}

static void UpdateSubtitle(struct pager *p)
{
	int range, win_h;

	win_h = getmaxy(p->pane.window);
	range = p->cfg->num_lines > win_h ?
	        p->cfg->num_lines - win_h : 0;
	SetWindowOffset(p, min(p->window_offset, range));
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

static void ScrollPager(struct pager *p, int dir)
{
	int win_h = getmaxy(p->pane.window);
	int max_offset = p->cfg->num_lines - win_h;
	int lineno;

	lineno = min((int) p->window_offset + dir, max_offset);
	SetWindowOffset(p, max(lineno, 0));
}

static bool MoveLinkCursor(struct pager *p, int key)
{
	struct pager_config *cfg = p->cfg;
	enum line_location line_loc;
	int new_link;

	if (cfg->num_links == 0) {
		return false;
	}

	switch (key) {
	case KEY_UP:
		new_link = cfg->current_link - 1;
		if (new_link < 0) {
			return false;
		}
		break;
	case KEY_DOWN:
		new_link = cfg->current_link + 1;
		if (new_link >= cfg->num_links) {
			return false;
		}
		break;
	default:
		return false;
	}

	// If we return true, the window will not be scrolled. If the next
	// link is within the current window, we jump to it and do not
	// scroll.
	// There is one corner case: if the next link is right on the very
	// edge of the window (one line above or below the current window,
	// represented by ON_WINDOW_EDGE), we want to both scroll *and*
	// move the selection to the link on the newly-revealed line.
	line_loc = LinkWithinWindow(p, new_link);
	if (line_loc != OUTSIDE_WINDOW) {
		cfg->current_link = new_link;
		return line_loc == INSIDE_WINDOW;
	}

	return false;
}

static void HandleKeypress(void *_p, int c)
{
	struct pager *p = _p;
	int win_h = getmaxy(p->pane.window);

	if (MoveLinkCursor(p, c)) {
		return;
	}

	switch (c) {
	case 'q':
	case 'Q':
		UI_ExitMainLoop();
		break;
	case KEY_UP:
		ScrollPager(p, -1);
		break;
	case KEY_DOWN:
		ScrollPager(p, 1);
		break;
	case KEY_PPAGE:
		ScrollPager(p, -win_h);
		break;
	case KEY_NPAGE:
		ScrollPager(p, win_h);
		break;
	case KEY_HOME:
		p->cfg->current_link = 0;
		SetWindowOffset(p, 0);
		break;
	case KEY_END:
		p->cfg->current_link = p->cfg->num_links - 1;
		ScrollPager(p, p->cfg->num_lines);
		break;
	case KEY_RESIZE:
		PagerMoved(p);
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

void P_RunPager(struct pager *p)
{
	struct saved_screen ss;
	struct pager *old_pager;

	old_pager = current_pager;
	current_pager = p;

	UI_SaveScreen(&ss);
	UI_SetTitleBar(p->cfg->title);
	UI_ActionsBarSetActions(p->cfg->actions);
	UI_PaneShow(p);
	UI_RunMainLoop();
	UI_PaneHide(p);
	UI_RestoreScreen(&ss);

	current_pager = old_pager;
}

void P_SwitchConfig(struct pager_config *cfg)
{
	assert(current_pager != NULL);
	current_pager->cfg = cfg;
	SetWindowOffset(current_pager, 0);
	P_ClearSearch(current_pager);
	UI_ActionsBarSetActions(cfg->actions);
}

void P_JumpToLine(struct pager *p, int lineno)
{
	int win_h;

	if (p->pane.window == NULL) {
		SetWindowOffset(p, lineno);
		return;
	}

	win_h = getmaxy(p->pane.window);
	SetWindowOffset(p, max(min(lineno, p->cfg->num_lines - win_h), 0));
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
