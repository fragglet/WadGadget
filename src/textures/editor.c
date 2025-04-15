//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "textures/editor.h"

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pager/pager.h"
#include "ui/pane.h"
#include "ui/stack.h"

enum {
	LINE_SPACE1,
	LINE_TX_NAME,
	LINE_TX_WIDTH,
	LINE_TX_HEIGHT,
	LINE_SPACE2,
	LINE_PATCH_HEADING,
	LINE_PATCH_START,
};

static void EditorGetLink(struct pager_config *cfg, int idx,
                          struct pager_link *link)
{
	switch (idx) {
	case 0:
		link->lineno = LINE_TX_NAME;
		return;
	case 1:
		link->lineno = LINE_TX_WIDTH;
		return;
	case 2:
		link->lineno = LINE_TX_HEIGHT;
		return;
	}
	link->lineno = LINE_PATCH_START + ((idx - 3) / 3);
	link->offset = idx % 3;
}

static void DrawField(WINDOW *win, int field_num, const char *fmt, ...)
{
	int curr_link = current_pager->cfg->current_link;
	va_list args;
	char buf[40];

	if (field_num == curr_link) {
		wattron(win, A_REVERSE);
	}

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);
	waddstr(win, buf);

	wattroff(win, A_REVERSE);
}

static void EditorDrawLine(WINDOW *win, unsigned int line, void *user_data)
{
	struct texture_editor *e = user_data;
	struct patch *patch;
	int patch_idx;
	char buf[40];

	switch (line) {
	case LINE_TX_NAME:
		wattron(win, A_BOLD);
		waddstr(win, "   Texture name: ");
		wattroff(win, A_BOLD);
		DrawField(win, 0, "%-8.8s", (*e->tx)->name);
		return;
	case LINE_TX_WIDTH:
		wattron(win, A_BOLD);
		waddstr(win, "          Width: ");
		wattroff(win, A_BOLD);
		DrawField(win, 1, "%-8d", (*e->tx)->width);
		return;
	case LINE_TX_HEIGHT:
		wattron(win, A_BOLD);
		waddstr(win, "         Height: ");
		wattroff(win, A_BOLD);
		DrawField(win, 2, "%-8d", (*e->tx)->width);
		return;
	case LINE_PATCH_HEADING:
		wattron(win, A_BOLD);
		snprintf(buf, sizeof(buf), "%16s%8s%8s",
		         "Patch name", "X", "Y");
		waddstr(win, buf);
		wattroff(win, A_BOLD);
		return;
	}

	if (line <= LINE_PATCH_HEADING) {
		return;
	}

	patch_idx = line - LINE_PATCH_START;
	patch = &(*e->tx)->patches[patch_idx];

	waddstr(win, "        ");
	// TODO: Real patch name
	DrawField(win, 3 + patch_idx * 3, "%8d", patch->patch);
	DrawField(win, 3 + patch_idx * 3 + 1, "%8d", patch->originx);
	DrawField(win, 3 + patch_idx * 3 + 2, "%8d", patch->originy);
}

static void EditorActivateLink(struct pager *p, int idx)
{
	// TODO: Change texture / patch properties
}

static const struct action *texture_editor_actions[] = {
    &exit_pager_action,
    NULL,
};

void TX_EditTexture(struct texture **tx, struct pnames *pnames)
{
	struct pager p;
	struct texture_editor e;
	struct pager_config cfg;

	e.tx = tx;
	e.pnames = pnames;

	memset(&cfg, 0, sizeof(struct pager_config));
	cfg.title = "Texture Editor (WIP)";
	cfg.draw_line = EditorDrawLine;
	cfg.user_data = &e;
	cfg.num_lines = LINE_PATCH_START + (*tx)->patchcount;
	cfg.actions = texture_editor_actions;
	cfg.get_link = EditorGetLink;
	cfg.activate_link = EditorActivateLink;
	cfg.num_links = (*tx)->patchcount * 3 + 3;

	P_InitPager(&p, &cfg);
	P_RunPager(&p, true);
}
