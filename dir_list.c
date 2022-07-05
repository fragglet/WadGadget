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
	struct directory_entry *files;
	unsigned int num_files;
};

static const char *GetEntry(struct blob_list *l, unsigned int idx)
{
	struct directory_listing *dir = (struct directory_listing *) l;
	if (idx >= dir->num_files) {
		return NULL;
	}
	return dir->files[idx].filename;
}

static enum blob_type GetEntryType(struct blob_list *l, unsigned int idx)
{
	struct directory_listing *dir = (struct directory_listing *) l;
	const struct directory_entry *ent;
	if (idx >= dir->num_files) {
		return BLOB_TYPE_FILE;
	}
	ent = &dir->files[idx];

	if (ent->is_subdirectory) {
		return BLOB_TYPE_DIR;
	} else {
		const char *extn = strlen(ent->filename) < 4 ? ""
		                 : ent->filename + strlen(ent->filename) - 4;
		if (!strcasecmp(extn, ".wad")) {
			return BLOB_TYPE_WAD;
		} else {
			return BLOB_TYPE_FILE;
		}
	}
}

static const char *GetEntryPath(struct blob_list *l, unsigned int idx)
{
	static char result_buf[128];
	struct directory_listing *dir = (struct directory_listing *) l;
	const struct directory_entry *ent;
	if (idx >= dir->num_files) {
		return NULL;
	}
	ent = &dir->files[idx];
	snprintf(result_buf, sizeof(result_buf),
	         "%s/%s", dir->bl.path, ent->filename);
	return result_buf;
}

static void FreeDirectory(struct blob_list *l)
{
	struct directory_listing *dir = (struct directory_listing *) l;
	int i;
	free(dir->bl.path);
	free(dir->bl.parent_dir);
	free(dir->bl.name);
	for (i = 0; i < dir->num_files; i++) {
		free(dir->files[i].filename);
	}
	free(dir->files);
	free(dir);
}

static int OrderByName(const void *x, const void *y)
{
	const struct directory_entry *dx = x, *dy = y;
	return strcasecmp(dx->filename, dy->filename);
}

struct directory_listing *DIR_ReadDirectory(const char *path)
{
	struct directory_listing *d;
	DIR *dir;

	d = calloc(1, sizeof(struct directory_listing));
	d->bl.get_entry_str = GetEntry;
	d->bl.get_entry_type = GetEntryType;
	d->bl.get_entry_path = GetEntryPath;
	d->bl.free = FreeDirectory;
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

	closedir(dir);

	qsort(d->files, d->num_files, sizeof(struct directory_entry),
	      OrderByName);

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

