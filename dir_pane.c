#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <dirent.h>

#include "dir_pane.h"
#include "ui.h"

struct directory_pane {
	struct list_pane pane;
	struct directory_listing *dir;
};

static const struct list_pane_action dir_to_wad[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Import"},
	{"F6", "Rename"},
	{"F7", "Mkdir"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

static const struct list_pane_action dir_to_dir[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Copy"},
	{"F6", "Rename"},
	{"F7", "Mkdir"},
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
			return dir_to_dir;

		case PANE_TYPE_WAD:
			return dir_to_wad;
	}
}

static const char *GetEntry(struct list_pane *l, unsigned int idx)
{
	struct directory_pane *p = (struct directory_pane *) l;
	const struct directory_entry *result = DIR_GetFile(p->dir, idx);
	if (result != NULL) {
		return result->filename;
	}
	return NULL;
}

static enum list_pane_entry_type GetEntryType(
	struct list_pane *l, unsigned int idx)
{
	struct directory_pane *p = (struct directory_pane *) l;
	const struct directory_entry *ent = DIR_GetFile(p->dir, idx);

	if (ent->is_subdirectory) {
		return PANE_ENTRY_DIR;
	} else {
		const char *extn = strlen(ent->filename) > 4 ? ""
		                 : ent->filename + strlen(ent->filename) - 4;
		if (!strcasecmp(extn, ".wad")) {
			return PANE_TYPE_WAD;
		} else {
			return PANE_ENTRY_FILE;
		}
	}
}

struct list_pane *UI_NewDirectoryPane(
	WINDOW *pane, struct directory_listing *dir)
{
	struct directory_pane *p;
	struct dirent *dirent;

	p = calloc(1, sizeof(struct directory_pane));
	p->pane.pane = pane;
	p->pane.parent_dir = ((struct blob_list *) dir)->parent_dir;
	p->pane.title = ((struct blob_list *) dir)->name;
	p->pane.type = PANE_TYPE_DIR;
	p->pane.get_actions = GetActions;
	p->pane.get_entry_str = GetEntry;
	p->pane.get_entry_type = GetEntryType;
	p->dir = dir;

	return &p->pane;
}

