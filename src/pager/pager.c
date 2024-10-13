//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "pager/pager.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdio.h>

#include "common.h"
#include "pager/help.h"
#include "ui/actions_bar.h"
#include "ui/dialog.h"
#include "ui/colors.h"
#include "ui/stack.h"
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

static void PerformPagerHelpAction(void)
{
	struct pager_config *cfg = current_pager->cfg;

	P_RunHelpPager(cfg->help_file);
}

const struct action pager_help_action = {
	KEY_F(1), 0, "Help", "Help", PerformPagerHelpAction,
};

static bool LinkWithinWindow(struct pager *p, int link)
{
	int win_h = getmaxy(p->pane.window);
	struct pager_link l;
	int lineno;

	if (link < 0 || link >= p->cfg->num_links) {
		return false;
	}

	p->cfg->get_link(p->cfg, link, &l);
	lineno = l.lineno;

	return lineno >= p->window_offset
	    && lineno < p->window_offset + win_h;
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

	if (LinkWithinWindow(p, cfg->current_link)) {
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

	if (LinkWithinWindow(p, new_link)) {
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
	int top_line, lines;

	UI_GetDesktopLines(&top_line, &lines);

	assert(wresize(p->pane.window, lines, COLS) == OK);
	assert(mvwin(p->pane.window, top_line, 0) == OK);
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

static struct pager_link GetLink(struct pager *p, int linknum)
{
	struct pager_link l;
	p->cfg->get_link(p->cfg, linknum, &l);
	return l;
}

static void UpKeypress(struct pager *p)
{
	struct pager_config *cfg = p->cfg;
	int new_link, old_lineno, new_lineno;

	if (cfg->num_links == 0) {
		ScrollPager(p, -1);
		return;
	}

	old_lineno = GetLink(p, cfg->current_link).lineno;

	// Scan backwards through the array until we find the first
	// link that is on a previous line.
	new_link = cfg->current_link;
	while (new_link > 0 && GetLink(p, new_link).lineno == old_lineno) {
		--new_link;
	}

	// We now know what line we're going to land on. Keep scanning
	// back through links on the same line until we find one that's
	// appropriate for the currently selected column.
	new_lineno = GetLink(p, new_link).lineno;
	while (new_link > 0
	    && GetLink(p, new_link - 1).lineno == new_lineno
	    && GetLink(p, new_link).column > cfg->current_column) {
		--new_link;
	}

	// We have a new link we'd like to jump to. But if the link is not
	// within the window, pressing up scrolls the window instead.
	if (new_link == cfg->current_link || !LinkWithinWindow(p, new_link)) {
		ScrollPager(p, -1);
	}

	// If we've scrolled up, the link *might* now be inside the window.
	if (LinkWithinWindow(p, new_link)) {
		cfg->current_link = new_link;
	}
}

static void DownKeypress(struct pager *p)
{
	struct pager_config *cfg = p->cfg;
	int new_link, old_lineno, new_lineno;

	if (cfg->num_links == 0) {
		ScrollPager(p, 1);
		return;
	}

	old_lineno = GetLink(p, cfg->current_link).lineno;

	// Scan forwards through the array until we find the first
	// link that is on a different line.
	new_link = cfg->current_link;
	while (new_link < cfg->num_links - 1
	    && GetLink(p, new_link).lineno == old_lineno) {
		++new_link;
	}

	// We now know what line we're going to land on. Keep scanning
	// forward through links on the same line until we find one that's
	// appropriate for the currently selected column.
	new_lineno = GetLink(p, new_link).lineno;
	while (new_link < cfg->num_links - 1) {
		struct pager_link l = GetLink(p, new_link + 1);
		if (l.lineno != new_lineno || l.column > cfg->current_column) {
			break;
		}
		++new_link;
	}

	// We have a new link we'd like to jump to. But if the link is not
	// within the window, pressing down scrolls the window instead.
	if (new_link == cfg->current_link || !LinkWithinWindow(p, new_link)) {
		ScrollPager(p, 1);
	}

	// If we've scrolled down, the link *might* now be inside the window.
	if (LinkWithinWindow(p, new_link)) {
		cfg->current_link = new_link;
	}
}

static void LeftKeypress(struct pager *p)
{
	struct pager_config *cfg = p->cfg;
	int lineno;

	if (cfg->num_links == 0) {
		return;
	}

	lineno = GetLink(p, cfg->current_link).lineno;
	if (cfg->current_link > 0
	 && GetLink(p, cfg->current_link - 1).lineno == lineno) {
		--cfg->current_link;
	}

	// If we scroll up/down, we want to aim for the same column.
	cfg->current_column = GetLink(p, cfg->current_link).column;
}

static void RightKeypress(struct pager *p)
{
	struct pager_config *cfg = p->cfg;
	int lineno;

	if (cfg->num_links == 0) {
		return;
	}

	lineno = GetLink(p, cfg->current_link).lineno;
	if (cfg->current_link < cfg->num_links - 1
	 && GetLink(p, cfg->current_link + 1).lineno == lineno) {
		++cfg->current_link;
	}

	cfg->current_column = GetLink(p, cfg->current_link).column;
}

static void HandleKeypress(void *_p, int c)
{
	struct pager *p = _p;
	int win_h = getmaxy(p->pane.window);

	switch (c) {
	case 'q':
	case 'Q':
		UI_ExitMainLoop();
		break;
	case KEY_UP:
		UpKeypress(p);
		break;
	case KEY_DOWN:
		DownKeypress(p);
		break;
	case KEY_LEFT:
		LeftKeypress(p);
		break;
	case KEY_RIGHT:
		RightKeypress(p);
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

	// Create the stack the pager will run in.
	p->stack = UI_NewStack();
	UI_SetCurrentStack(p->stack);
	UI_SetTitleBar(p->cfg->title);
	UI_ActionsBarSetActions(p->cfg->actions);
	UI_ActionsBarEnable(true);
	UI_ActionsBarSetFunctionKeys(false);
	UI_PaneShow(p);
}

void P_FreePager(struct pager *p)
{
	UI_PaneHide(p);
	UI_FreeStack(p->stack);
	delwin(p->line_win);
	delwin(p->pane.window);
	delwin(p->search_pad);
	free(p->last_search);
}

void P_OpenPager(struct pager *p)
{
	struct pane_stack *old_active = UI_ActiveStack();
	UI_AddStack(p->stack);
	UI_SetActiveStack(old_active);
}

void P_ClosePager(struct pager *p)
{
	UI_RemoveStack(p->stack);
}

void P_RunPager(struct pager *p, bool fullscreen)
{
	struct pager *old_pager;

	old_pager = current_pager;
	current_pager = p;

	// On small screen sizes, the pager can get very cramped. In this
	// case we automatically run full screen instead.
	// TODO: This should really be based on the "predicted" height of
	// the stack if it runs split.
	if (fullscreen || LINES < 40) {
		UI_SetFullscreenStack(p->stack);
	} else {
		UI_AddStack(p->stack);
	}
	UI_RunMainLoop();
	UI_RemoveStack(p->stack);

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

	// We need to scroll if the line is outside the window, but we
	// also jump if the location is in the bottom half of the screen.
	// If the line is (for example) on the last line of the screen,
	// we want to be able to see the surrounding context.
	if (lineno < p->window_offset
	 || lineno >= p->window_offset + win_h / 2) {
		P_JumpToLine(current_pager, lineno - 5);
	}
}

void P_ClearSearch(struct pager *p)
{
	current_pager->search_line = -1;
}
