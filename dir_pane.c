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
#include "strings.h"
#include "ui.h"

struct directory_pane {
	struct blob_list_pane pane;
	struct directory_listing *dir;
};

static const struct blob_list_pane_action dir_to_wad[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Import"},
	{"F6", "Rename"},
	{"F7", "Mkdir"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

static const struct blob_list_pane_action dir_to_dir[] = {
	{"F3", "View"},
	{"F4", "Edit"},
	{"F5", "> Copy"},
	{"F6", "Rename"},
	{"F7", "Mkdir"},
	{"F8", "Delete"},
	{"F9", "Make WAD"},
	{NULL, NULL},
};

static const struct blob_list_pane_action *GetActions(
	struct blob_list_pane *other)
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

	while (p->pane.pane.selected > 0
	    && DIR_GetFile(p->dir, p->pane.pane.selected - 1) == NULL) {
		--p->pane.pane.selected;
	}
}

static void Keypress(void *directory_pane, int key)
{
	struct directory_pane *p = directory_pane;
	char *input_filename;
	int selected = UI_BlobListPaneSelected(&p->pane);

	if (key == KEY_F(6) && selected >= 0) {
		char *old_name = DIR_GetFile(p->dir, selected)->name;
		char *new_name;
		input_filename = UI_TextInputDialogBox(
		    "Rename", 30, "New name for '%s'?", old_name);
		if (input_filename == NULL) {
			return;
		}
		old_name = StringJoin("/", DIR_GetPath(p->dir), old_name, NULL);
		new_name = StringJoin("/", DIR_GetPath(p->dir),
		                      input_filename, NULL);
		rename(old_name, new_name);
		free(old_name);
		free(new_name);
		free(input_filename);
		RefreshDir(p);
		return;
	}
	if (key == KEY_F(7)) {
		char *filename;
		input_filename = UI_TextInputDialogBox(
		    "Make directory", 30, "Name for new directory?");
		if (input_filename == NULL) {
			return;
		}
		filename = StringJoin("/", DIR_GetPath(p->dir),
		                      input_filename, NULL);
		mkdir(filename, 0777);
		free(input_filename);
		free(filename);
		RefreshDir(p);
		return;
	}
	// TODO: Delete all marked
	if (key == KEY_F(8) && selected >= 0) {
		char *filename;
		filename = DIR_GetFile(p->dir, selected)->name;
		if (!UI_ConfirmDialogBox("Confirm Delete", "Delete file '%s'?",
		                         filename)) {
			return;
		}
		filename = StringJoin("/", DIR_GetPath(p->dir), filename, NULL);
		remove(filename);
		free(filename);
		RefreshDir(p);
		return;
	}

	UI_BlobListPaneKeypress(directory_pane, key);
}

struct blob_list_pane *UI_NewDirectoryPane(
	WINDOW *w, struct directory_listing *dir)
{
	struct directory_pane *p;

	p = calloc(1, sizeof(struct directory_pane));
	UI_BlobListPaneInit(&p->pane, w);
	// TODO: Set the window title
	p->pane.pane.pane.keypress = Keypress;
	p->pane.type = PANE_TYPE_DIR;
	p->pane.blob_list = (struct blob_list *) dir;
	p->pane.get_actions = GetActions;
	p->dir = dir;
	// Select first item (assuming there is one):
	UI_BlobListPaneKeypress(&p->pane, KEY_DOWN);

	return &p->pane;
}

