#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <dirent.h>

#include "ui.h"
#include "list_pane.h"
#include "wad_pane.h"

struct directory_pane {
	struct list_pane pane;
	const char **files;
	unsigned int num_files;
};

static const char *GetEntry(struct list_pane *l, unsigned int idx)
{
	struct directory_pane *p = (struct directory_pane *) l;
	if (idx >= p->num_files) {
		return NULL;
	}
	return p->files[idx];
}

struct directory_pane *UI_NewDirectoryPane(WINDOW *pane, const char *path)
{
	struct directory_pane *p;
	DIR *dir;
	struct dirent *dirent;

	p = calloc(1, sizeof(struct directory_pane));
	p->pane.pane = pane;
	p->pane.title = strdup(path);
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
		if (!strcmp(dirent->d_name, ".")) {
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

	return p;
}

void UI_DrawDirectoryPane(struct directory_pane *p)
{
	UI_DrawListPane(&p->pane);
}

