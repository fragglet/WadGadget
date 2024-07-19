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
#include "ui/stack.h"
#include "ui/title_bar.h"

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

static bool IterateLinks(struct help_pager_config *cfg, int *lineno, int *col,
                         struct pager_link *link)
{
	const char *line;

	while (*lineno < cfg->pc.num_lines) {
		line = cfg->lines[*lineno];
		while (line[*col] != '\0') {
			if (IsLinkStart(line + *col)) {
				link->lineno = *lineno;
				link->column = *col;
				// Skip past link text:
				*col = strstr(line + *col, "](") - line;
				return true;
			}
			++*col;
		}

		*col = 0;
		++*lineno;
	}

	return false;
}

static void FindLinks(struct help_pager_config *cfg)
{
	int lineno = 0, col = 0, linknum;
	struct pager_link l;
	int num_links = 0;

	while (IterateLinks(cfg, &lineno, &col, &l)) {
		++num_links;
	}

	cfg->links = checked_calloc(num_links, sizeof(struct help_pager_config));
	lineno = 0, col = 0, linknum = 0;
	while (IterateLinks(cfg, &lineno, &col, &l)) {
		cfg->links[linknum] = l;
		++linknum;
	}

	cfg->pc.num_links = num_links;
}

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
	free(cfg->links);
}

static void SaveToHistory(struct pager *p, struct help_pager_config *cfg)
{
	struct help_pager_history *h;

	h = checked_calloc(1, sizeof(struct help_pager_history));
	h->filename = checked_strdup(cfg->filename);
	h->window_offset = p->window_offset;
	h->current_link = cfg->pc.current_link;
	h->current_column = cfg->pc.current_column;
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

// A level-1 heading on the first line is used to set the window title.
static void SetTitle(struct help_pager_config *cfg)
{
	if (cfg->pc.num_lines < 1
	 || !StringHasPrefix(cfg->lines[0], "# ")) {
		cfg->pc.title = checked_strdup("WadGadget Help");
		return;
	}

	cfg->pc.title =
		StringJoin(": ", "WadGadget Help", cfg->lines[0] + 2, NULL);
	free(cfg->lines[0]);
	memmove(cfg->lines, cfg->lines + 1,
	        (cfg->pc.num_lines - 1) * sizeof(char *));
	--cfg->pc.num_lines;
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
	SetTitle(cfg);
	UnindentLines(cfg);
	FindLinks(cfg);
	cfg->pc.current_link = 0;
	cfg->pc.current_column = 0;

	if (current_pager != NULL && current_pager->cfg == &cfg->pc) {
		UI_SetTitleBar(cfg->pc.title);
	}

	return true;
}

static void PerformFollowLink(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;
	struct pager_link *curr_link;
	const char *line, *link_middle;
	char *filename, *anchor = NULL, *p;

	if (cfg->pc.current_link < 0
	 || cfg->pc.current_link >= cfg->pc.num_links) {
		return;
	}

	curr_link = &cfg->links[cfg->pc.current_link];
	line = cfg->lines[curr_link->lineno];

	link_middle = strstr(line + curr_link->column, "](");
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
		UI_ShowNotice("There is nothing go back to.");
		return;
	}

	h = cfg->history;
	cfg->history = h->next;

	OpenHelpFile(cfg, h->filename);
	current_pager->window_offset = h->window_offset;
	cfg->pc.current_link = h->current_link;
	current_pager->search_line = -1;

	free(h->filename);
	free(h);
}

static const struct action back_action = {
	0, 'B', "Back", "Back", PerformGoBack,
};

static void OpenTableOfContents(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;

	if (!strcmp(cfg->filename, "contents.md")) {
		UI_ShowNotice("You are already viewing the table of contents.");
		return;
	}

	SaveToHistory(current_pager, cfg);
	OpenHelpFile(cfg, "contents.md");
	current_pager->window_offset = 0;
	current_pager->search_line = -1;
}

static const struct action toc_action = {
	0, 'T', "Contents", "Table of Contents", OpenTableOfContents,
};

static void OpenHelpOnHelp(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;

	if (!strcmp(cfg->filename, "help.md")) {
		UI_ShowNotice("You are already viewing the help page.");
		return;
	}

	SaveToHistory(current_pager, cfg);
	OpenHelpFile(cfg, "help.md");
	current_pager->window_offset = 0;
	current_pager->search_line = -1;
}

static const struct action help_on_help_action = {
	KEY_F(1), 'H', "Help", "Help", OpenHelpOnHelp,
};

static const struct action *help_pager_actions[] = {
	&exit_pager_action,
	&help_on_help_action,
	&back_action,
	&pager_prev_link_action,
	&pager_next_link_action,
	&follow_link_action,
	&toc_action,
	&pager_search_action,
	&pager_search_again_action,
	NULL,
};

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
	struct pager_link *curr_link = NULL;
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

	if (cfg->pc.current_link >= 0
	 && cfg->pc.current_link < cfg->pc.num_links) {
		curr_link = &cfg->links[cfg->pc.current_link];
	}

	for (p = line; *p != '\0'; ++p) {
		if (IsLinkStart(p)) {
			bool is_curr_link = curr_link != NULL
			                 && lineno == curr_link->lineno
			                 && (p - line) == curr_link->column;
			p = DrawLink(win, p, is_curr_link);
		} else if (HaveSyntaxElements(p, "**", "**", NULL)) {
			p = DrawBoldText(win, p);
		} else {
			waddch(win, *p);
		}
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

static void HelpPagerGetLink(struct pager_config *_cfg, int idx,
                             struct pager_link *result)
{
	struct help_pager_config *cfg = _cfg->user_data;
	assert(idx >= 0 && idx < cfg->pc.num_links);

	*result = cfg->links[idx];
}

bool P_InitHelpConfig(struct help_pager_config *cfg, const char *filename)
{
	cfg->pc.title = NULL;
	cfg->pc.draw_line = DrawHelpLine;
	cfg->pc.get_link = HelpPagerGetLink;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = help_pager_actions;
	cfg->lines = NULL;
	cfg->links = NULL;
	cfg->filename = NULL;
	cfg->history = NULL;
	cfg->pc.num_lines = 0;
	cfg->pc.num_links = 0;

	return OpenHelpFile(cfg, filename);
}

bool P_RunHelpPager(const char *filename)
{
	struct help_pager_config cfg;
	struct pager p;

	if (!P_InitHelpConfig(&cfg, filename)) {
		return false;
	}

	P_InitPager(&p, &cfg.pc);
	P_RunPager(&p, false);
	P_FreePager(&p);
	P_FreeHelpConfig(&cfg);

	return true;
}
