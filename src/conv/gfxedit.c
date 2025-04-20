//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

// #include "conv/gfxedit.h"

#include <curses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "conv/graphic.h"
#include "pager/pager.h"
#include "ui/actions_bar.h"
#include "ui/dialog.h"
#include "ui/pane.h"
#include "ui/title_bar.h"

enum {
	LINE_SPACE1,
	LINE_LUMP_NAME,
	LINE_SPACE2,
	LINE_GFX_WIDTH,
	LINE_GFX_HEIGHT,
	LINE_GFX_XOFF,
	LINE_GFX_YOFF,
	NUM_LINES,
};

enum {
	FIELD_GFX_WIDTH,
	FIELD_GFX_HEIGHT,
	FIELD_GFX_XOFF,
	FIELD_GFX_YOFF,
	NUM_FIELDS,
};

struct gfx_editor {
	struct pager_config cfg;
	struct patch_header hdr;
	uint8_t *lump;
	size_t lump_len;
	bool edited;
};

static int field_linenos[] = {
	LINE_GFX_WIDTH,
	LINE_GFX_HEIGHT,
	LINE_GFX_XOFF,
	LINE_GFX_YOFF,
};

static void EditorGetLink(struct pager_config *cfg, int idx,
                          struct pager_link *link)
{
	link->lineno = field_linenos[idx];
	link->offset = 0;
}

static void DrawField(struct gfx_editor *e, WINDOW *win, int field_num,
                      const char *fmt, ...)
{
	int curr_link = e->cfg.current_link;
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
	struct gfx_editor *e = user_data;

	switch (line) {
	case LINE_LUMP_NAME:
		wattron(win, A_BOLD);
		waddstr(win, " Lump: ");
		wattroff(win, A_BOLD);
		break;
	case LINE_GFX_WIDTH:
		wattron(win, A_BOLD);
		waddstr(win, "     Width: ");
		wattroff(win, A_BOLD);
		DrawField(e, win, FIELD_GFX_WIDTH, "%-8d", e->hdr.width);
		break;
	case LINE_GFX_HEIGHT:
		wattron(win, A_BOLD);
		waddstr(win, "    Height: ");
		wattroff(win, A_BOLD);
		DrawField(e, win, FIELD_GFX_HEIGHT, "%-8d", e->hdr.height);
		break;
	case LINE_GFX_XOFF:
		wattron(win, A_BOLD);
		waddstr(win, "  X Offset: ");
		wattroff(win, A_BOLD);
		DrawField(e, win, FIELD_GFX_XOFF, "%-8d", e->hdr.leftoffset);
		break;
	case LINE_GFX_YOFF:
		wattron(win, A_BOLD);
		waddstr(win, "  Y Offset: ");
		wattroff(win, A_BOLD);
		DrawField(e, win, FIELD_GFX_YOFF, "%-8d", e->hdr.topoffset);
		break;
	}
}

static bool EditField(struct gfx_editor *e, const char *prompt,
                      int16_t *field, int min)
{
	char *answer;
	int val;

	answer = UI_TextInputDialogBox("Edit field", "Edit", 6, prompt);
	if (answer == NULL) {
		return false;
	}

	val = atoi(answer);
	free(answer);
	if (val < min || val > 16384) {
		UI_ShowNotice("Value not in range.");
		return false;
	}

	*field = (int16_t) val;
	e->edited = true;
	return true;
}

static void EditorActivateLink(struct pager *p, int idx)
{
	struct gfx_editor *e = current_pager->cfg->user_data;

	switch (idx) {
	case FIELD_GFX_WIDTH:
		EditField(e, "Enter new graphic width:",
		          (int16_t *) e->lump, 0);
		return;
	case FIELD_GFX_HEIGHT:
		EditField(e, "Enter new graphic height:",
		          (int16_t *) e->lump, 0);
		return;
	case FIELD_GFX_XOFF:
		EditField(e, "Enter new X offset:",
		          (int16_t *) e->lump, 0);
		return;
	case FIELD_GFX_YOFF:
		EditField(e, "Enter new Y offset:",
		          (int16_t *) e->lump, 0);
		return;
	}
}

static const struct action *gfx_editor_actions[] = {
    &exit_pager_action,  NULL,
};

bool V_EditGraphic(uint8_t *lump, size_t lump_len)
{
	struct pager p;
	struct gfx_editor e;

	assert(lump_len >= 8);

	e.lump = lump;
	e.lump_len = lump_len;

	memcpy(&e.hdr, lump, sizeof(struct patch_header));
	V_SwapPatchHeader(&e.hdr);

	memset(&e.cfg, 0, sizeof(struct pager_config));
	e.cfg.title = "Graphic Editor";
	e.cfg.draw_line = EditorDrawLine;
	e.cfg.help_file = NULL;
	e.cfg.user_data = &e;
	e.cfg.actions = gfx_editor_actions;
	e.cfg.get_link = EditorGetLink;
	e.cfg.activate_link = EditorActivateLink;
	e.cfg.num_lines = NUM_LINES;
	e.cfg.num_links = NUM_FIELDS;

	P_InitPager(&p, &e.cfg);
	P_RunPager(&p, true);

	return e.edited;
}
