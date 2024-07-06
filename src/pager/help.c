//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// Help viewer. This implements a very restricted subset of the Markdown
// syntax.

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "common.h"
#include "fs/vfile.h"
#include "help_text.h"
#include "pager/pager.h"
#include "pager/plaintext.h"
#include "pager/help.h"
#include "stringlib.h"
#include "ui/actions_bar.h"
#include "ui/dialog.h"

static const char *HelpFileContents(const char *filename)
{
	int i;

	for (i = 0; help_files[i].filename != NULL; i++) {
		if (!strcmp(help_files[i].filename, filename)) {
			return help_files[i].contents;
		}
	}

	return NULL;
}

static bool HaveSyntaxElements(const char *start, const char *el1, ...)
{
	va_list args;
	const char *p, *q;

	if (strncmp(start, el1, strlen(el1)) != 0) {
		return false;
	}

	p = start + strlen(el1);

	va_start(args, el1);
	for (;;) {
		const char *el = va_arg(args, const char *);
		if (el == NULL) {
			return true;
		}

		q = strstr(p, el);
		if (q == NULL) {
			return false;
		}
		p = q + strlen(el);
	}
}

static bool IsLinkStart(const char *p)
{
	return HaveSyntaxElements(p, "[", "](", ")", NULL);
}

static const char *LineHasLink(const char *p)
{
	char *q = strstr(p, "[");
	if (q != NULL && IsLinkStart(q)) {
		return p;
	}
	return NULL;
}

static int ScanForLink(struct help_pager_config *cfg, int lineno, int dir)
{
	while (lineno >= 0 && lineno < cfg->pc.num_lines) {
		if (LineHasLink(cfg->lines[lineno])) {
			return lineno;
		}

		lineno += dir;
	}

	return -1;
}

static void PerformNextLink(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;
	int lineno;

	lineno = ScanForLink(cfg, cfg->current_link_line + 1, 1);
	if (lineno < 0) {
		lineno = ScanForLink(cfg, 0, 1);
	}

	if (lineno >= 0) {
		cfg->current_link_line = lineno;
		if (current_pager != NULL) {
			P_JumpWithinWindow(current_pager, lineno);
		}
	}
}

static const struct action next_link_action = {
        '\t', 0, "NextLink", "Next Link", PerformNextLink,
};

static void PerformPrevLink(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;
	int lineno;

	lineno = ScanForLink(cfg, cfg->current_link_line - 1, -1);
	if (lineno < 0) {
		lineno = ScanForLink(cfg, cfg->current_link_line - 1, -1);
	}

	if (lineno >= 0) {
		cfg->current_link_line = lineno;
		if (current_pager != NULL) {
			P_JumpWithinWindow(current_pager, lineno);
		}
	}
}

static const struct action prev_link_action = {
	KEY_BTAB, 0, NULL, NULL, PerformPrevLink,
};

static char *AnchorName(const char *line)
{
	char *result, *p, *q;

	// Only headings.
	if (!StringHasPrefix(line, "#")) {
		return NULL;
	}

	while (*line == '#' || *line == ' ') {
		++line;
	}

	result = checked_strdup(line);
	for (p = result, q = result; *p != '\0'; ++p) {
		if (isalnum(*p)) {
			*q = tolower(*p);
			++q;
		} else if (*p == ' ' || *p == '-') {
			*q = '-';
			++q;
		}
	}
	*q = '\0';

	return result;
}

static int JumpToAnchor(struct pager *p, const char *anchor)
{
	struct help_pager_config *cfg = p->cfg->user_data;
	char *curr;
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		curr = AnchorName(cfg->lines[i]);
		if (curr != NULL) {
			if (!strcasecmp(curr, anchor)) {
				free(curr);
				P_JumpWithinWindow(p, i);
				return i;
			}
			free(curr);
		}
	}

	return -1;
}

static void FreeLines(struct help_pager_config *cfg)
{
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		free(cfg->lines[i]);
	}
	free(cfg->lines);
}

static void SaveToHistory(struct pager *p, struct help_pager_config *cfg)
{
	struct help_pager_history *h;

	h = checked_calloc(1, sizeof(struct help_pager_history));
	h->filename = checked_strdup(cfg->filename);
	h->window_offset = p->window_offset;
	h->current_link_line = cfg->current_link_line;
	h->next = cfg->history;
	cfg->history = h;
}

// After loading a help file we scan through and unindent all code
// blocks (lines starting with four spaces). This is so that eg. the
// title ASCII art for the Unofficial Doom Specs fits in 80 columns.
static void UnindentLines(struct help_pager_config *cfg)
{
	bool maybe_code_block = true;
	char *line;
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		line = cfg->lines[i];
		if (strlen(line) == 0) {
			maybe_code_block = true;
		} else if (strncmp(line, "    ", 4) != 0) {
			maybe_code_block = false;
		} else if (maybe_code_block) {
			memmove(line, line + 3, strlen(line) - 2);
		}
	}
}

static bool OpenHelpFile(struct help_pager_config *cfg, const char *filename)
{
	const char *contents = HelpFileContents(filename);
	if (contents == NULL) {
		UI_MessageBox("Can't find help file '%s'", filename);
		return false;
	}

	FreeLines(cfg);
	free(cfg->filename);
	cfg->filename = checked_strdup(filename);
	cfg->lines = P_PlaintextLines(contents, strlen(contents),
	                              &cfg->pc.num_lines);
	UnindentLines(cfg);
	cfg->current_link_line = ScanForLink(cfg, 0, 1);

	return true;
}

static void PerformFollowLink(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;
	const char *line = cfg->lines[cfg->current_link_line];
	const char *link_middle = strstr(line, "](");
	char *filename, *anchor = NULL, *p;

	if (link_middle == NULL) {
		return;
	}

	filename = checked_strdup(link_middle + 2);
	p = strchr(filename, ')');
	if (p == NULL) {
		return;
	}

	*p = '\0';

	p = strchr(filename, '#');
	if (p != NULL) {
		anchor = p + 1;
		*p = '\0';
	}

	SaveToHistory(current_pager, cfg);

	if (strlen(filename) > 0) {
		OpenHelpFile(cfg, filename);
		current_pager->window_offset = 0;
		current_pager->search_line = -1;
		P_ClearSearch(current_pager);
	}
	if (anchor != NULL) {
		int anchor_line = JumpToAnchor(current_pager, anchor);
		if (anchor_line >= 0) {
			current_pager->search_line = anchor_line;
		}
	}
	free(filename);
}

static const struct action follow_link_action = {
	'\r', 0, "Open", "Open Link", PerformFollowLink,
};

static void PerformGoBack(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;
	struct help_pager_history *h;

	if (cfg->history == NULL) {
		return;
	}

	h = cfg->history;
	cfg->history = h->next;

	OpenHelpFile(cfg, h->filename);
	current_pager->window_offset = h->window_offset;
	cfg->current_link_line = h->current_link_line;
	current_pager->search_line = -1;

	free(h->filename);
	free(h);
}

static const struct action back_action = {
	KEY_LEFT, 'B', "Back", "Back", PerformGoBack,
};

static void OpenTableOfContents(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;

	SaveToHistory(current_pager, cfg);
	OpenHelpFile(cfg, "contents.md");
	current_pager->window_offset = 0;
	current_pager->search_line = -1;
}

static const struct action toc_action = {
	0, 'T', "Contents", "Table of Contents", OpenTableOfContents,
};


static const struct action *help_pager_actions[] = {
	&exit_pager_action,
	&back_action,
	&prev_link_action,
	&next_link_action,
	&follow_link_action,
	&toc_action,
	&pager_search_action,
	&pager_search_again_action,
	NULL,
};

enum line_location { OUTSIDE_WINDOW, INSIDE_WINDOW, ON_WINDOW_EDGE };

static enum line_location LineWithinWindow(struct pager *p, int lineno)
{
	int win_h = getmaxy(p->pane.window);
	if (lineno == -1) {
		return OUTSIDE_WINDOW;
	} else if (lineno == p->window_offset - 1
	 || lineno == p->window_offset + win_h) {
		return ON_WINDOW_EDGE;
	} else if (lineno >= p->window_offset
	        && lineno < p->window_offset + win_h) {
		return INSIDE_WINDOW;
	} else {
		return OUTSIDE_WINDOW;
	}
}

static const char *DrawLink(WINDOW *win, const char *link, bool highlighted)
{
	const char *p;

	if (highlighted) {
		wattron(win, A_REVERSE);
	}

	wattron(win, A_BOLD);
	wattron(win, A_UNDERLINE);
	for (p = link + 1; *p != '\0'; ++p) {
		if (HaveSyntaxElements(p, "](", ")", NULL)) {
			p = strstr(p, ")");
			break;
		}
		waddch(win, *p);
	}
	wattroff(win, A_UNDERLINE);
	wattroff(win, A_BOLD);
	wattroff(win, A_REVERSE);

	return p;
}

static const char *DrawBoldText(WINDOW *win, const char *text)
{
	const char *p, *end;

	text += 2;
	end = strstr(text, "**");
	if (end == NULL) {
		return text;
	}

	wattron(win, A_BOLD);
	for (p = text; p < end; ++p) {
		waddch(win, *p);
	}
	wattroff(win, A_BOLD);

	return end + 1;
}

static void DrawHelpLine(WINDOW *win, unsigned int lineno, void *user_data)
{
	struct help_pager_config *cfg = user_data;
	const char *line, *p;

	assert(lineno < cfg->pc.num_lines);
	line = cfg->lines[lineno];

	if (StringHasPrefix(line, "#")) {
		wattron(win, A_BOLD);
		wattron(win, A_UNDERLINE);
		while (*line == '#' || *line == ' ') {
			++line;
		}
	} else {
		wattroff(win, A_BOLD);
		wattroff(win, A_UNDERLINE);
	}

	for (p = line; *p != '\0'; ++p) {
		if (IsLinkStart(p)) {
			// Note we only allow one link per line.
			p = DrawLink(win, p, lineno == cfg->current_link_line);
		} else if (HaveSyntaxElements(p, "**", "**", NULL)) {
			p = DrawBoldText(win, p);
		} else {
			waddch(win, *p);
		}
	}
}

static bool HelpPagerKeypress(struct pager *p, int key)
{
	struct help_pager_config *cfg = p->cfg->user_data;
	enum line_location line_loc;
	int lineno;

	switch (key) {
	case KEY_UP:
		lineno = ScanForLink(cfg, cfg->current_link_line - 1, -1);
		break;
	case KEY_DOWN:
		lineno = ScanForLink(cfg, cfg->current_link_line + 1, 1);
		break;
	case KEY_HOME:
		cfg->current_link_line = ScanForLink(cfg, 0, 1);
		return false;
	case KEY_END:
		cfg->current_link_line =
			ScanForLink(cfg, cfg->pc.num_lines - 1, -1);
		return false;
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
	line_loc = LineWithinWindow(p, lineno);
	if (line_loc != OUTSIDE_WINDOW) {
		cfg->current_link_line = lineno;
		return line_loc == INSIDE_WINDOW;
	}

	return false;
}

static void HelpPagerMoved(struct pager *p)
{
	struct help_pager_config *cfg = p->cfg->user_data;
	int win_h = getmaxy(p->pane.window);
	int lineno;

	if (cfg->current_link_line < 0
	 || LineWithinWindow(p, cfg->current_link_line) == INSIDE_WINDOW) {
		return;
	}

	// Currently selected link is not within the visible window, so
	// we should try to find a new link.

	if (cfg->current_link_line < p->window_offset) {
		lineno = ScanForLink(cfg, p->window_offset, 1);
	} else {
		lineno = ScanForLink(cfg, p->window_offset + win_h - 1, -1);
	}

	if (LineWithinWindow(p, lineno) == INSIDE_WINDOW) {
		cfg->current_link_line = lineno;
	}
}

void P_FreeHelpConfig(struct help_pager_config *cfg)
{
	struct help_pager_history *h;
	FreeLines(cfg);
	free(cfg->filename);

	h = cfg->history;
	while (h != NULL) {
		struct help_pager_history *next = h->next;
		free(h->filename);
		free(h);
		h = next;
	}
}

bool P_InitHelpConfig(struct help_pager_config *cfg, const char *filename)
{
	cfg->pc.title = "WadGadget help";
	cfg->pc.draw_line = DrawHelpLine;
	cfg->pc.keypress = HelpPagerKeypress;
	cfg->pc.window_moved = HelpPagerMoved;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = help_pager_actions;
	cfg->lines = NULL;
	cfg->filename = NULL;
	cfg->history = NULL;
	cfg->pc.num_lines = 0;

	return OpenHelpFile(cfg, filename);
}

bool P_RunHelpPager(const char *filename)
{
	struct help_pager_config cfg;

	if (!P_InitHelpConfig(&cfg, filename)) {
		return false;
	}

	P_RunPager(&cfg.pc);
	P_FreeHelpConfig(&cfg);

	return true;
}
