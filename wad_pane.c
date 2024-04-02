#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "dialog.h"
#include "ui.h"
#include "blob_list_pane.h"
#include "wad_pane.h"

struct wad_pane {
	struct blob_list_pane pane;
	struct wad_file *f;
};

static const struct blob_list_pane_action wad_to_wad[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Copy"},
	{"F6", "Rename"},
	{"F7", "New lump"},
	{"F8", "Delete"},
	{NULL, NULL},
};

static const struct blob_list_pane_action wad_to_dir[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Export"},
	{"F6", "Rename"},
	{"F7", "New lump"},
	{"F8", "Delete"},
	{"F9", "> Export as WAD"},
	{NULL, NULL},
};

static const struct blob_list_pane_action *GetActions(struct blob_list_pane *other)
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
	int selected = UI_BlobListPaneSelected(&p->pane);

	if (key == KEY_F(6) && selected >= 0) {
		char *name = UI_TextInputDialogBox(
		    "Rename lump", 8,
		    "Enter new name for lump:");
		if (name == NULL) {
			return;
		}
		W_SetLumpName(p->f, selected, name);
		free(name);
	}

	if (key == KEY_F(7)) {
		char *name = UI_TextInputDialogBox(
		    "New lump", 8,
		    "Enter name for new lump:");
		if (name == NULL) {
			return;
		}
		W_AddEntries(p->f, selected, 1);
		W_SetLumpName(p->f, selected, name);
		UI_BlobListPaneKeypress(&p->pane, KEY_DOWN);
		return;
	}

	// TODO: Delete multiple
	if (key == KEY_F(8) && selected >= 0) {
		if (UI_ConfirmDialogBox(
		     "Confirm delete",
		     "Delete lump named '%.8s'?",
		     W_GetDirectory(p->f)[selected].name)) {
			W_DeleteEntry(p->f, selected);
			UI_BlobListPaneKeypress(&p->pane, KEY_UP);
		}
		return;
	}

	UI_BlobListPaneKeypress(wad_pane, key);
}

struct blob_list_pane *UI_NewWadPane(WINDOW *w, struct wad_file *f)
{
	struct wad_pane *p;
	p = checked_calloc(1, sizeof(struct wad_pane));
	UI_BlobListPaneInit(&p->pane, w);
	// TODO: Set the window title
	p->pane.pane.pane.keypress = Keypress;
	p->pane.type = PANE_TYPE_WAD;
	p->pane.blob_list = (struct blob_list *) f;
	p->pane.get_actions = GetActions;
	p->f = f;
	// Select first item (assuming there is one):
	UI_BlobListPaneKeypress(&p->pane, KEY_DOWN);
	return &p->pane;
}

