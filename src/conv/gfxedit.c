//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "conv/gfxedit.h"

#include <assert.h>
#include <curses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser/actions.h"
#include "common.h"
#include "conv/graphic.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "fs/wad_file.h"
#include "pager/pager.h"
#include "ui/actions_bar.h"
#include "ui/dialog.h"
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
	struct directory *dir;
	struct wad_file *wf;
	unsigned int lump_index;
	uint16_t orig_width;
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

// TODO: This global shouldn't exist:
static struct gfx_editor *curr_editor;

static void EditorGetLink(struct pager_config *cfg, int idx,
                          struct pager_link *link)
{
	link->lineno = field_linenos[idx];
	link->offset = 0;
}

static void DrawField(struct gfx_editor *e, WINDOW *win, int field_num,
                      const char *fmt, ...) PRINTF_ATTRIBUTE(4, 5);

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
	struct wad_file_entry *dir;
	char buf[10];

	switch (line) {
	case LINE_LUMP_NAME:
		// TODO: Allow lump to be renamed?
		wattron(win, A_BOLD);
		waddstr(win, "      Lump: ");
		wattroff(win, A_BOLD);
		dir = W_GetDirectory(e->wf);
		snprintf(buf, sizeof(buf), "%-.8s", dir[e->lump_index].name);
		waddstr(win, buf);
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

static bool EditField(struct gfx_editor *e, const char *prompt, int16_t *field,
                      int min)
{
	char *answer;
	int val;

	answer = UI_TextInputDialogBox("Edit field", "Edit", 6, prompt);
	if (answer == NULL) {
		return false;
	}

	val = atoi(answer);
	free(answer);
	if (val < min || val > INT16_MAX) {
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
	uint16_t prev_width;

	if (!B_CheckReadOnly(e->dir)) {
		return;
	}

	switch (idx) {
	case FIELD_GFX_WIDTH:
		prev_width = e->hdr.width;
		// As with NewWadTool, we allow not only the offsets to be
		// edited but the width and height as well. For the most
		// part this is safe to do; the one exception is if we try
		// to increase the width to be greater than the original.
		if (EditField(e, "Enter new graphic width:",
		              (int16_t *) &e->hdr.width, 0) &&
		    e->hdr.width > e->orig_width &&
		    !UI_ConfirmDialogBox(
		        "Confirm new width", "Edit", "Cancel",
		        "New width is wider than the original width.\n"
		        "This may make the graphic corrupt. Proceed?")) {
			e->hdr.width = prev_width;
		}
		return;
	case FIELD_GFX_HEIGHT:
		EditField(e, "Enter new graphic height:",
		          (int16_t *) &e->hdr.height, 0);
		return;
	case FIELD_GFX_XOFF:
		EditField(e,
		          "Enter new X offset:", (int16_t *) &e->hdr.leftoffset,
		          INT16_MIN);
		return;
	case FIELD_GFX_YOFF:
		EditField(e,
		          "Enter new Y offset:", (int16_t *) &e->hdr.topoffset,
		          INT16_MIN);
		return;
	}
}

static const struct action edit_field_action = {
    '\r', 0, "Edit", "Edit", P_ActionOpenLink,
};

static void ActionAutoCenter(void)
{
	uint8_t *srcbuf = curr_editor->lump;
	uint32_t *columnofs =
	    (uint32_t *) (srcbuf + sizeof(struct patch_header));
	uint32_t off;
	unsigned int accum = 0, total_pixels = 0;
	int x, post_len;

	if (!B_CheckReadOnly(curr_editor->dir)) {
		return;
	}

	for (x = 0; x < curr_editor->hdr.width; ++x) {
		off = columnofs[x];
		SwapLE32(&off);
		while (srcbuf[off] != 0xff) {
			post_len = srcbuf[off + 1];
			off += 4 + post_len;
			accum += (x + 1) * post_len;
			total_pixels += post_len;
		}
	}

	if (total_pixels == 0) {
		curr_editor->hdr.leftoffset = curr_editor->hdr.width / 2;
	} else {
		curr_editor->hdr.leftoffset = accum / total_pixels;
	}

	curr_editor->edited = true;
}

static const struct action center_xoff_action = {
    KEY_F(5), 'C', "Center", "Auto-Center", ActionAutoCenter,
};

static void ActionFloat(void)
{
	if (!B_CheckReadOnly(curr_editor->dir)) {
		return;
	}

	curr_editor->hdr.topoffset = curr_editor->hdr.height + 10;
	curr_editor->edited = true;
}

static const struct action float_action = {
    KEY_F(6), 'F', "Float", "Float", ActionFloat,
};

static void ActionGround(void)
{
	if (!B_CheckReadOnly(curr_editor->dir)) {
		return;
	}

	curr_editor->hdr.topoffset = max(curr_editor->hdr.height, 3) - 3;
	curr_editor->edited = true;
}

static const struct action on_ground_action = {
    KEY_F(7), 'G', "Ground", "Ground", ActionGround,
};

static const struct action *gfx_editor_actions[] = {
    &exit_pager_action,
    &pager_help_action,
    &edit_field_action,
    &center_xoff_action,
    &float_action,
    &on_ground_action,
    NULL,
};

static void LoadLump(struct gfx_editor *e)
{
	VFILE *in = W_OpenLump(e->wf, e->lump_index);
	e->lump = vfreadall(in, &e->lump_len);
	vfclose(in);
	assert(e->lump_len >= sizeof(struct patch_header));

	e->hdr = *((struct patch_header *) e->lump);
	V_SwapPatchHeader(&e->hdr);
	e->orig_width = e->hdr.width;
}

static void SaveLump(struct gfx_editor *e)
{
	VFILE *out;

	*((struct patch_header *) e->lump) = e->hdr;
	V_SwapPatchHeader((struct patch_header *) e->lump);

	out = W_OpenLumpRewrite(e->wf, e->lump_index);
	assert(vfwrite(e->lump, 1, e->lump_len, out) == e->lump_len);
	vfclose(out);
}

bool V_EditGraphic(struct directory *dir, struct directory_entry *ent)
{
	struct pager p;
	struct gfx_editor e;

	e.dir = dir;
	e.wf = VFS_WadFile(dir);
	e.lump_index = ent - dir->entries;
	e.edited = false;
	LoadLump(&e);

	memset(&e.cfg, 0, sizeof(struct pager_config));
	e.cfg.title = "Graphic Editor";
	e.cfg.draw_line = EditorDrawLine;
	e.cfg.help_file = "gfx_editor.md";
	e.cfg.user_data = &e;
	e.cfg.actions = gfx_editor_actions;
	e.cfg.get_link = EditorGetLink;
	e.cfg.activate_link = EditorActivateLink;
	e.cfg.num_lines = NUM_LINES;
	e.cfg.num_links = NUM_FIELDS;

	P_InitPager(&p, &e.cfg);

	curr_editor = &e;
	P_RunPager(&p, true);

	if (e.edited) {
		SaveLump(&e);
	}

	free(e.lump);

	return e.edited;
}
