#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <dirent.h>

#include "dir_list.h"
#include "ui.h"

struct directory_listing {
	struct blob_list bl;
	char *parent_dir;
	char *filename;
	struct directory_entry *files;
	unsigned int num_files;
};

struct directory_listing *DIR_ReadDirectory(const char *path)
{
	struct directory_listing *d;
	DIR *dir;

	d = calloc(1, sizeof(struct directory_listing));
	dir = opendir(path);
	assert(dir != NULL);

	BL_SetPathFields(&d->bl, path);
	d->files = NULL;
	d->num_files = 0;
	for (;;)
	{
		struct dirent *dirent = readdir(dir);
		struct directory_entry *ent;
		char *path;
		if (dirent == NULL) {
			break;
		}
		if (dirent->d_name[0] == '.') {
			continue;
		}
		path = strdup(dirent->d_name);
		assert(path != NULL);

		d->files = realloc(d->files,
			sizeof(struct directory_entry) * (d->num_files + 1));
		assert(d->files != NULL);
		ent = &d->files[d->num_files];
		ent->filename = path;
		ent->is_subdirectory = dirent->d_type == DT_DIR;
		++d->num_files;
	}

	return d;
}

const struct directory_entry *DIR_GetFile(
	struct directory_listing *dir, unsigned int file_index)
{
	if (file_index >= dir->num_files) {
		return NULL;
	}
	return &dir->files[file_index];
}

