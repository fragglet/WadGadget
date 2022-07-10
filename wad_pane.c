#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "dialog.h"
#include "ui.h"
#include "list_pane.h"
#include "wad_pane.h"

struct wad_pane {
	struct list_pane pane;
	struct wad_file *f;
};

static const struct list_pane_action wad_to_wad[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Copy"},
	{"F6", "Rename"},
	{"F7", "New lump"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

static const struct list_pane_action wad_to_dir[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Export"},
	{"F6", "Rename"},
	{"F7", "New lump"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

static const struct list_pane_action *GetActions(struct list_pane *other)
{
	switch (other->type) {
		case PANE_TYPE_DIR:
			return wad_to_dir;

		case PANE_TYPE_WAD:
			return wad_to_wad;

		default:
			return NULL;
	}
}

static void Keypress(void *wad_pane, int key)
{
	struct wad_pane *p = wad_pane;
	unsigned int selected = p->pane.selected;

	if (key == KEY_F(8) && selected > 0) {
		if (UI_ConfirmDialogBox(
		     "Confirm delete",
		     "Delete lump named '%.8s'?",
		     W_GetDirectory(p->f)[selected - 1].name)) {
			W_DeleteEntry(p->f, selected - 1);
		}
		return;
	}

	UI_ListPaneKeypress(wad_pane, key);
}

struct list_pane *UI_NewWadPane(WINDOW *w, struct wad_file *f)
{
	struct wad_pane *p;
	p = checked_calloc(1, sizeof(struct wad_pane));
	UI_ListPaneInit(&p->pane, w);
	p->pane.pane.keypress = Keypress;
	p->pane.type = PANE_TYPE_WAD;
	p->pane.blob_list = (struct blob_list *) f;
	p->pane.get_actions = GetActions;
	p->pane.selected = min(1, W_NumLumps(f));
	p->f = f;
	return &p->pane;
}

