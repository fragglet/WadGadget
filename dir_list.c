#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "dir_list.h"
#include "ui.h"

struct directory_listing {
	struct blob_list bl;
	struct blob_list_entry *files;
	char *path;
	unsigned int num_files;
};

static const struct blob_list_entry *GetEntry(
	struct blob_list *l, unsigned int idx)
{
	struct directory_listing *dir = (struct directory_listing *) l;
	if (idx >= dir->num_files) {
		return NULL;
	}
	return &dir->files[idx];
}

static const char *GetEntryPath(struct blob_list *l, unsigned int idx)
{
	static char result_buf[128];
	struct directory_listing *dir = (struct directory_listing *) l;
	const struct blob_list_entry *ent;
	if (idx >= dir->num_files) {
		return NULL;
	}
	ent = &dir->files[idx];
	snprintf(result_buf, sizeof(result_buf),
	         "%s/%s", dir->bl.path, ent->name);
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
		free(dir->files[i].name);
	}
	free(dir->files);
	free(dir);
}

static int OrderByName(const void *x, const void *y)
{
	const struct blob_list_entry *dx = x, *dy = y;
	return strcasecmp(dx->name, dy->name);
}

static int HasWadExtension(char *name)
{
	char *extn;
	if (strlen(name) < 4) {
		return 0;
	}
	extn = name + strlen(name) - 4;
	return !strcasecmp(extn, ".wad");
}

static void FreeEntries(struct directory_listing *d)
{
	int i;

	for (i = 0; i < d->num_files; i++) {
		free(d->files[i].name);
	}

	free(d->files);
	d->files = NULL;
	d->num_files = 0;
}

void DIR_RefreshDirectory(struct directory_listing *d)
{
	DIR *dir = opendir(d->path);
	assert(dir != NULL);

	FreeEntries(d);
	BL_SetPathFields(&d->bl, d->path);
	for (;;)
	{
		struct dirent *dirent = readdir(dir);
		struct blob_list_entry *ent;
		char *path;
		if (dirent == NULL) {
			break;
		}
		if (dirent->d_name[0] == '.') {
			continue;
		}
		path = checked_strdup(dirent->d_name);

		d->files = checked_realloc(d->files,
			sizeof(struct blob_list_entry) * (d->num_files + 1));
		ent = &d->files[d->num_files];
		ent->name = path;
		ent->type = dirent->d_type == DT_DIR ? BLOB_TYPE_DIR :
		            HasWadExtension(ent->name) ? BLOB_TYPE_WAD :
		            BLOB_TYPE_FILE;
		++d->num_files;
	}

	closedir(dir);

	qsort(d->files, d->num_files, sizeof(struct blob_list_entry),
	      OrderByName);
}

struct directory_listing *DIR_ReadDirectory(const char *path)
{
	struct directory_listing *d;

	d = calloc(1, sizeof(struct directory_listing));
	d->bl.get_entry = GetEntry;
	d->bl.get_entry_path = GetEntryPath;
	d->bl.free = FreeDirectory;
	d->path = checked_strdup(path);
	d->files = NULL;
	d->num_files = 0;

	DIR_RefreshDirectory(d);

	return d;
}

const struct blob_list_entry *DIR_GetFile(
	struct directory_listing *dir, unsigned int file_index)
{
	if (file_index >= dir->num_files) {
		return NULL;
	}
	return &dir->files[file_index];
}

unsigned int DIR_NumFiles(struct directory_listing *dir)
{
	return dir->num_files;
}

