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
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "pager/pager.h"
#include "ui/dialog.h"
#include "ui/list_pane.h"
#include "ui/pane.h"
#include "ui/stack.h"
#include "ui/title_bar.h"

#define TX(e) ((e)->b->txs->textures[(e)->texture_index])

enum {
	LINE_SPACE1,
	LINE_TX_NAME,
	LINE_TX_WIDTH,
	LINE_TX_HEIGHT,
	LINE_SPACE2,
	LINE_PATCH_HEADING,
	LINE_PATCH_START,
};

enum {
	FIELD_TX_NAME,
	FIELD_TX_WIDTH,
	FIELD_TX_HEIGHT,
};

struct pname_selector {
	struct list_pane lp;
	struct texture_bundle *b;
	bool selected;
};

static void PnameSelectorDrawElement(WINDOW *win, int index, void *data)
{
	struct pname_selector *s = data;
	char buf[10];
	if (s->lp.active && index == s->lp.selected) {
		wattron(win, A_REVERSE);
	}
	snprintf(buf, sizeof(buf), "%-8.8s", s->b->pn->pnames[index]);
	mvwaddstr(win, 0, 0, buf);
	wattroff(win, A_REVERSE);
}

static unsigned int PnameSelectorNumEntries(void *data)
{
	struct pname_selector *s = data;
	return s->b->pn->num_pnames;
}

static const struct list_pane_funcs pname_select_funcs = {
    PnameSelectorDrawElement,
    PnameSelectorNumEntries,
};

static void PnameSelectorKeypress(void *p, int key)
{
	struct pname_selector *s = p;

	switch (key) {
	case '\r':
		s->selected = true;
		UI_ExitMainLoop();
		return;
	case 27:
		UI_ExitMainLoop();
		return;
	default:
		UI_ListPaneKeypress(p, key);
		return;
	}
}

static int SelectPname(struct texture_bundle *b)
{
	const struct action **saved_actions;
	struct pname_selector s;
	WINDOW *win = newwin(LINES - 3, 40, 1, 40);

	s.b = b;
	s.selected = false;
	UI_ListPaneInit(&s.lp, win, &pname_select_funcs, &s);
	s.lp.pane.keypress = PnameSelectorKeypress;
	UI_ListPaneSetTitle(&s.lp, "Select a patch:");
	saved_actions = UI_ActionsBarSetActions(NULL);
	UI_PaneShow(&s);
	UI_RunMainLoop();
	UI_PaneHide(&s);
	UI_ActionsBarSetActions(saved_actions);
	UI_ListPaneFree(&s.lp);
	delwin(win);

	if (s.selected) {
		return s.lp.selected;
	} else {
		return -1;
	}
}

static void EditorGetLink(struct pager_config *cfg, int idx,
                          struct pager_link *link)
{
	switch (idx) {
	case FIELD_TX_NAME:
		link->lineno = LINE_TX_NAME;
		return;
	case FIELD_TX_WIDTH:
		link->lineno = LINE_TX_WIDTH;
		return;
	case FIELD_TX_HEIGHT:
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
		DrawField(win, FIELD_TX_NAME, "%-8.8s", TX(e)->name);
		return;
	case LINE_TX_WIDTH:
		wattron(win, A_BOLD);
		waddstr(win, "          Width: ");
		wattroff(win, A_BOLD);
		DrawField(win, FIELD_TX_WIDTH, "%-8d", TX(e)->width);
		return;
	case LINE_TX_HEIGHT:
		wattron(win, A_BOLD);
		waddstr(win, "         Height: ");
		wattroff(win, A_BOLD);
		DrawField(win, FIELD_TX_HEIGHT, "%-8d", TX(e)->height);
		return;
	case LINE_PATCH_HEADING:
		wattron(win, A_BOLD);
		snprintf(buf, sizeof(buf), "%17s%8s%8s", "Patch name", "X",
		         "Y");
		waddstr(win, buf);
		wattroff(win, A_BOLD);
		return;
	}

	if (line <= LINE_PATCH_HEADING) {
		return;
	}

	patch_idx = line - LINE_PATCH_START;
	patch = &TX(e)->patches[patch_idx];

	waddstr(win, "         ");
	DrawField(win, 3 + patch_idx * 3, "%-8.8s",
	          e->b->pn->pnames[patch->patch]);
	DrawField(win, 3 + patch_idx * 3 + 1, "%8d", patch->originx);
	DrawField(win, 3 + patch_idx * 3 + 2, "%8d", patch->originy);
}

static bool EditField(struct texture_editor *e, const char *prompt,
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
	++e->b->txs->modified_count;
	return true;
}

static void EditorActivateLink(struct pager *p, int idx)
{
	struct texture_editor *e = current_pager->cfg->user_data;
	struct patch *patch;
	int pname_idx;
	char *new_name;

	switch (idx) {
	case FIELD_TX_NAME:
		new_name = UI_TextInputDialogBox("Edit field", "Edit", 8,
		                                 "Enter new texture name:");
		if (new_name == NULL || strlen(new_name) == 0) {
			free(new_name);
			return;
		}
		if (!TX_RenameTexture(e->b->txs, e->texture_index, new_name)) {
			UI_ShowNotice(
			    "There is already a texture with that name.");
		}
		free(new_name);
		return;
	case FIELD_TX_WIDTH:
		EditField(e,
		          "Enter new texture width:", (int16_t *) &TX(e)->width,
		          1);
		return;
	case FIELD_TX_HEIGHT:
		EditField(e, "Enter new texture height:",
		          (int16_t *) &TX(e)->height, 1);
		return;
	}

	patch = &TX(e)->patches[(idx - 3) / 3];
	switch (idx % 3) {
	case 0:
		pname_idx = SelectPname(e->b);
		if (pname_idx >= 0) {
			patch->patch = pname_idx;
			++e->b->txs->modified_count;
		}
		return;
	case 1:
		if (EditField(e, "Enter new X offset:", &patch->originx,
		              -16384)) {
			++current_pager->cfg->current_link;
		}
		return;
	case 2:
		EditField(e, "Enter new Y offset:", &patch->originy, -16384);
		return;
	}
}

const struct action edit_field_action = {
    '\r', 0, "Edit", "Edit", P_PerformOpenLink,
};

static void UpdatePagerConfig(struct pager_config *cfg,
                              struct texture_editor *e)
{
	cfg->num_lines = LINE_PATCH_START + TX(e)->patchcount;
	cfg->num_links = TX(e)->patchcount * 3 + 3;
}

static void PerformAddPatch(void)
{
	struct texture_editor *e = current_pager->cfg->user_data;
	struct patch p;
	int curr_link = current_pager->cfg->current_link;
	int insert_index, patch_num;

	patch_num = SelectPname(e->b);
	if (patch_num < 0) {
		return;
	}

	if (curr_link < 3) {
		insert_index = 0;
	} else {
		insert_index = curr_link / 3;
	}

	p.patch = patch_num;
	p.originx = 0;
	p.originy = 0;
	TX(e) = TX_InsertPatch(TX(e), insert_index, &p);

	UpdatePagerConfig(current_pager->cfg, e);

	// Select X offset on newly-added patch, so user can edit it:
	current_pager->cfg->current_link = insert_index * 3 + 4;
}

const struct action add_patch_action = {
    KEY_F(7), 'K', "AddPatch", "Add Patch", PerformAddPatch,
};

static void PerformDeletePatch(void)
{
	struct texture_editor *e = current_pager->cfg->user_data;
	struct texture *tx;
	int patch_index, curr_link = current_pager->cfg->current_link;

	if (curr_link < 3) {
		return;
	}

	patch_index = (curr_link - 3) / 3;
	tx = TX(e);

	memmove(&tx->patches[patch_index], &tx->patches[patch_index + 1],
	        sizeof(struct patch) * (tx->patchcount - patch_index - 1));
	--tx->patchcount;
	UpdatePagerConfig(current_pager->cfg, e);

	current_pager->cfg->current_link =
	    min(current_pager->cfg->current_link,
	        current_pager->cfg->num_links - 1);
}

const struct action delete_patch_action = {
    KEY_F(8), 'X', "DelPatch", "Delete Patch", PerformDeletePatch,
};

static const struct action *texture_editor_actions[] = {
    &exit_pager_action,
    &edit_field_action,
    &add_patch_action,
    &delete_patch_action,
    NULL,
};

bool TX_EditTexture(struct texture_bundle *b, int texture_index)
{
	struct pager p;
	struct texture_editor e;
	struct pager_config cfg;

	e.b = b;
	e.texture_index = texture_index;

	memset(&cfg, 0, sizeof(struct pager_config));
	cfg.title = "Texture Editor (WIP)";
	cfg.draw_line = EditorDrawLine;
	cfg.user_data = &e;
	cfg.actions = texture_editor_actions;
	cfg.get_link = EditorGetLink;
	cfg.activate_link = EditorActivateLink;
	UpdatePagerConfig(&cfg, &e);

	P_InitPager(&p, &cfg);
	P_RunPager(&p, true);

	return e.edited;
}
