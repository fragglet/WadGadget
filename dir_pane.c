#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "dialog.h"
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
		case PANE_TYPE_DIR:
			return dir_to_dir;

		case PANE_TYPE_WAD:
			return dir_to_wad;

		default:
			return NULL;
	}
}

static void RefreshDir(struct directory_pane *p)
{
	DIR_RefreshDirectory(p->dir);

	while (p->pane.selected > 0
	    && DIR_GetFile(p->dir, p->pane.selected - 1) == NULL) {
		--p->pane.selected;
	}
}

static void Keypress(void *directory_pane, int key)
{
	struct directory_pane *p = directory_pane;
	unsigned int selected = p->pane.selected;

	// TODO: Operate on absolute paths, not cwd
	if (key == KEY_F(6) && selected > 0) {
		char *old_name = DIR_GetFile(p->dir, selected-1)->name;
		char *new_name = UI_TextInputDialogBox(
		    "Rename", 30, "New name for '%s'?", old_name);
		if (new_name == NULL) {
			return;
		}
		rename(old_name, new_name);
		free(new_name);
		RefreshDir(p);
		return;
	}
	if (key == KEY_F(7) && selected > 0) {
		char *filename = UI_TextInputDialogBox(
		    "Make directory", 30, "Name for new directory?");
		if (filename == NULL) {
			return;
		}
		mkdir(filename, 0777);
		free(filename);
		RefreshDir(p);
		return;
	}
	// TODO: Delete all marked
	if (key == KEY_F(8) && selected > 0) {
		char *filename = DIR_GetFile(p->dir, selected-1)->name;
		if (!UI_ConfirmDialogBox("Confirm Delete", "Delete file '%s'?",
		                         filename)) {
			return;
		}
		remove(filename);
		RefreshDir(p);
		return;
	}

	UI_ListPaneKeypress(directory_pane, key);
}

struct list_pane *UI_NewDirectoryPane(
	WINDOW *w, struct directory_listing *dir)
{
	struct directory_pane *p;

	p = calloc(1, sizeof(struct directory_pane));
	UI_ListPaneInit(&p->pane, w);
	p->pane.pane.keypress = Keypress;
	p->pane.type = PANE_TYPE_DIR;
	p->pane.blob_list = (struct blob_list *) dir;
	p->pane.get_actions = GetActions;
	p->pane.selected = min(1, DIR_NumFiles(dir));
	p->dir = dir;

	return &p->pane;
}

