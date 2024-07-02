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

static bool ScanNextLink(struct help_pager_config *cfg)
{
	char *p;
	int lineno = cfg->current_link_line;

	while (lineno < cfg->pc.num_lines) {
		p = strstr(cfg->lines[lineno], "[");
		if (p != NULL && IsLinkStart(p)) {
			cfg->current_link_line = lineno;
			return true;
		}
		++lineno;
	}

	return false;
}

static void JumpNextLink(struct help_pager_config *cfg)
{
	++cfg->current_link_line;
	if (ScanNextLink(cfg)) {
		return;
	}

	cfg->current_link_line = 0;
	ScanNextLink(cfg);
}

static void PerformNextLink(void)
{
	JumpNextLink(current_pager->cfg->user_data);
}

static const struct action next_link_action = {
        '\t', 0, "Next Link", "Next Link", PerformNextLink,
};

static const struct action *help_pager_actions[] = {
	&exit_pager_action,
	&next_link_action,
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
			continue;
		}
		waddch(win, *p);
	}
}

void P_FreeHelpConfig(struct help_pager_config *cfg)
{
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		free(cfg->lines[i]);
	}
	free(cfg->lines);
}

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

bool P_InitHelpConfig(struct help_pager_config *cfg, const char *filename)
{
	const char *contents;

	cfg->pc.title = "WadGadget help";
	cfg->pc.draw_line = DrawHelpLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = help_pager_actions;

	contents = HelpFileContents(filename);
	if (contents == NULL) {
		UI_MessageBox("Can't find help file '%s'", filename);
		return false;
	}

	cfg->lines = P_PlaintextLines(contents, strlen(contents),
	                              &cfg->pc.num_lines);
	cfg->current_link_line = -1;
	JumpNextLink(cfg);
	return true;
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
