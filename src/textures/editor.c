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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pager/pager.h"
#include "ui/pane.h"
#include "ui/stack.h"

static void EditorGetLink(struct pager_config *cfg, int idx,
                          struct pager_link *link)
{
	link->lineno = idx / 3;
	link->offset = idx % 3;
}

static void EditorDrawLine(WINDOW *win, unsigned int line, void *user_data)
{
	struct texture_editor *e = user_data;
	int curr_link = current_pager->cfg->current_link;
	struct patch *patch;
	int field_num;
	char buf[40];

	if ((curr_link / 3) == line) {
		field_num = curr_link % 3;
	} else {
		field_num = -1;
	}

	waddstr(win, "    ");

	// Name (texture or patch name):
	if (line == 0) {
		snprintf(buf, sizeof(buf), "%-8.8s", (*e->tx)->name);
		wattron(win, A_BOLD);
	} else {
		patch = &(*e->tx)->patches[line - 1];
		// TODO: Patch name, not number:
		snprintf(buf, sizeof(buf), "%8d", patch->patch);
		wattroff(win, A_BOLD);
	}

	if (field_num == 0) {
		wattron(win, A_REVERSE);
	}
	waddstr(win, buf);
	wattroff(win, A_REVERSE);

	// X offset / width:
	if (line == 0) {
		snprintf(buf, sizeof(buf), "%8d", (*e->tx)->width);
	} else {
		snprintf(buf, sizeof(buf), "%8d", patch->originx);
	}

	if (field_num == 1) {
		wattron(win, A_REVERSE);
	}
	waddstr(win, buf);
	wattroff(win, A_REVERSE);

	// Y offset / width:
	if (line == 0) {
		snprintf(buf, sizeof(buf), "%8d", (*e->tx)->width);
	} else {
		snprintf(buf, sizeof(buf), "%8d", patch->originx);
	}

	if (field_num == 2) {
		wattron(win, A_REVERSE);
	}
	waddstr(win, buf);
	wattroff(win, A_REVERSE);
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
	cfg.num_lines = (*tx)->patchcount + 1;
	cfg.actions = texture_editor_actions;
	cfg.get_link = EditorGetLink;
	cfg.activate_link = EditorActivateLink;
	cfg.num_links = cfg.num_lines * 3;

	P_InitPager(&p, &cfg);
	P_RunPager(&p, true);
}
