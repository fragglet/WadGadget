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

static bool ScanNextLink(struct help_pager_config *cfg, int dir)
{
	int lineno = cfg->current_link_line;

	while (lineno >= 0 && lineno < cfg->pc.num_lines) {
		if (LineHasLink(cfg->lines[lineno])) {
			cfg->current_link_line = lineno;
			if (current_pager != NULL) {
				P_JumpWithinWindow(current_pager, lineno);
			}
			return true;
		}

		lineno += dir;
	}

	return false;
}

static void JumpNextLink(struct help_pager_config *cfg)
{
	++cfg->current_link_line;
	if (ScanNextLink(cfg, 1)) {
		return;
	}

	cfg->current_link_line = 0;
	ScanNextLink(cfg, 1);
}

static void JumpPrevLink(struct help_pager_config *cfg)
{
	--cfg->current_link_line;
	if (ScanNextLink(cfg, -1)) {
		return;
	}

	cfg->current_link_line = cfg->pc.num_lines - 1;
	ScanNextLink(cfg, -1);
}

static void PerformNextLink(void)
{
	JumpNextLink(current_pager->cfg->user_data);
}

static const struct action next_link_action = {
        '\t', 0, "NextLink", "Next Link", PerformNextLink,
};

static void PerformPrevLink(void)
{
	JumpPrevLink(current_pager->cfg->user_data);
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

static bool JumpToAnchor(struct pager *p, const char *anchor)
{
	struct help_pager_config *cfg = p->cfg->user_data;
	char *curr;
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		curr = AnchorName(cfg->lines[i]);
		if (curr != NULL) {
			if (!strcasecmp(curr, anchor)) {
				free(curr);
				P_JumpToLine(p, i);
				return true;
			}
			free(curr);
		}
	}

	return false;
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
	cfg->current_link_line = -1;
	JumpNextLink(cfg);

	return true;
}

static void PerformFollowLink(void)
{
	struct help_pager_config *cfg = current_pager->cfg->user_data;
	const char *line = cfg->lines[cfg->current_link_line];
	const char *link_middle = strstr(line, "](");
	char *filename, *p;

	if (link_middle == NULL) {
		return;
	}

	filename = checked_strdup(link_middle + 2);
	p = strchr(filename, ')');
	if (p == NULL) {
		return;
	}

	*p = '\0';

	if (filename[0] == '#') {
		JumpToAnchor(current_pager, filename + 1);
	} else {
		SaveToHistory(current_pager, cfg);
		OpenHelpFile(cfg, filename);
		current_pager->window_offset = 0;
		P_ClearSearch(current_pager);
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
