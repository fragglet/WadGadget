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
	int win_h = 25;
	int lineno = cfg->current_link_line;

	if (current_pager != NULL && current_pager->pane.window != NULL) {
		win_h = getmaxy(current_pager->pane.window);
	}

	while (lineno >= 0 && lineno < cfg->pc.num_lines) {
		if (!LineHasLink(cfg->lines[lineno])) {
			lineno += dir;
			continue;
		}

		// This line has a link.
		cfg->current_link_line = lineno;
		if (current_pager != NULL &&
		    (lineno < current_pager->window_offset
		  || lineno >= current_pager->window_offset + win_h)) {
			P_JumpToLine(current_pager, lineno - 5);
		}
		return true;
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
        '\t', 0, "Next Link", "Next Link", PerformNextLink,
};

static void PerformPrevLink(void)
{
	JumpPrevLink(current_pager->cfg->user_data);
}

static const struct action prev_link_action = {
	KEY_BTAB, 0, NULL, NULL, PerformPrevLink,
};

static void FreeLines(struct help_pager_config *cfg)
{
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		free(cfg->lines[i]);
	}
	free(cfg->lines);
}

static bool OpenHelpFile(struct help_pager_config *cfg, const char *filename)
{
	const char *contents = HelpFileContents(filename);
	if (contents == NULL) {
		UI_MessageBox("Can't find help file '%s'", filename);
		return false;
	}

	FreeLines(cfg);
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

	filename = strdup(link_middle + 2);
	p = strchr(filename, ')');
	if (p == NULL) {
		return;
	}

	*p = '\0';
	OpenHelpFile(cfg, filename);
	free(filename);
}

static const struct action follow_link_action = {
	'\r', 0, "Open Link", "Open Link", PerformFollowLink,
};

static const struct action *help_pager_actions[] = {
	&exit_pager_action,
	&prev_link_action,
	&next_link_action,
	&follow_link_action,
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

	if (StringHasPrefix(line, "# ")) {
		wattron(win, A_BOLD);
		wattron(win, A_UNDERLINE);
		line += 2;
	} else if (StringHasPrefix(line, "## ")) {
		wattron(win, A_BOLD);
		wattron(win, A_UNDERLINE);
		line += 3;
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
	FreeLines(cfg);
}

bool P_InitHelpConfig(struct help_pager_config *cfg, const char *filename)
{
	cfg->pc.title = "WadGadget help";
	cfg->pc.draw_line = DrawHelpLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = help_pager_actions;
	cfg->lines = NULL;
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
