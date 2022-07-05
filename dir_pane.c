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
	const char **files;
	unsigned int num_files;
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
	if (idx >= p->num_files) {
		return NULL;
	}
	return p->files[idx];
}

struct list_pane *UI_NewDirectoryPane(WINDOW *pane, const char *path)
{
	struct directory_pane *p;
	DIR *dir;
	struct dirent *dirent;

	p = calloc(1, sizeof(struct directory_pane));
	p->pane.pane = pane;
	p->pane.title = strdup(path);
	p->pane.type = PANE_TYPE_DIR;
	p->pane.get_actions = GetActions;
	p->pane.get_entry_str = GetEntry;

	dir = opendir(path);
	assert(dir != NULL);

	p->files = NULL;
	for (p->num_files = 0;;)
	{
		struct dirent *dirent = readdir(dir);
		char *path;
		if (dirent == NULL) {
			break;
		}
		if (dirent->d_name[0] == '.'
		 && strcmp(dirent->d_name, "..") != 0) {
			continue;
		}
		path = strdup(dirent->d_name);
		assert(path != NULL);

		p->files = realloc(
			p->files, sizeof(char *) * (p->num_files + 1));
		assert(p->files != NULL);
		p->files[p->num_files] = path;
		++p->num_files;
	}

	return &p->pane;
}

