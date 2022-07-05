#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
		case PANE_TYPE_NONE:
			return NULL;

		case PANE_TYPE_DIR:
			return wad_to_dir;

		case PANE_TYPE_WAD:
			return wad_to_wad;
	}
}

static const char *GetEntry(struct list_pane *l, unsigned int idx)
{
	static char buf[9];
	struct wad_pane *p = (struct wad_pane *) l;
	const struct wad_file_entry *directory = W_GetDirectory(p->f);
	if (idx >= W_NumLumps(p->f)) {
		return NULL;
	}
	snprintf(buf, sizeof(buf), "%-8s", directory[idx].name);
	return buf;
}

static enum list_pane_entry_type GetEntryType(
	struct list_pane *l, unsigned int idx)
{
	return PANE_ENTRY_LUMP;
}

struct list_pane *UI_NewWadPane(WINDOW *pane, struct wad_file *f)
{
	struct wad_pane *p;
	p = calloc(1, sizeof(struct wad_pane));
	p->pane.pane = pane;
	p->pane.type = PANE_TYPE_WAD;
	p->pane.parent_dir = ((struct blob_list *) f)->parent_dir;
	p->pane.title = ((struct blob_list *) f)->name;
	p->pane.get_entry_str = GetEntry;
	p->pane.get_actions = GetActions;
	p->pane.get_entry_type = GetEntryType;
	p->f = f;
	return &p->pane;
}

