//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#include "common.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "stringlib.h"

const struct file_type file_type_dir = {"directory"};
const struct file_type file_type_file = {"file"};

static int HasWadExtension(const char *name)
{
	const char *extn;
	if (strlen(name) < 4) {
		return 0;
	}
	extn = name + strlen(name) - 4;
	return !strcasecmp(extn, ".wad");
}

static int OrderByName(const void *x, const void *y)
{
	const struct directory_entry *dx = x, *dy = y;
	// Directories get listed before files.
	int cmp = (dy->type == &file_type_dir) - (dx->type == &file_type_dir);
	if (cmp != 0) {
		return cmp;
	}
	return strcasecmp(dx->name, dy->name);
}

static bool _RealDirRefresh(struct directory *d,
                            struct directory_entry **entries,
                            size_t *num_entries)
{
	DIR *dir;

	*entries = NULL;
	*num_entries = 0;

	dir = opendir(d->path);
	if (dir == NULL) {
		return false;
	}

	for (;;) {
		struct dirent *dirent = readdir(dir);
		struct directory_entry *ent;
		struct stat s;
		bool stat_ok;
		char *path;

		if (dirent == NULL) {
			break;
		}
		if (dirent->d_name[0] == '.') {
			continue;
		}
		path = StringJoin("/", d->path, dirent->d_name, NULL);
		// We stat() the file, which resolves symlinks and gives
		// additional information such as file size and type
		// (in a portable way)
		stat_ok = stat(path, &s) == 0;
		free(path);
		path = checked_strdup(dirent->d_name);

		*entries =
		    checked_realloc(*entries, sizeof(struct directory_entry) *
		                                  (*num_entries + 1));
		ent = *entries + *num_entries;
		ent->name = path;
		ent->type = stat_ok && S_ISDIR(s.st_mode) ? &file_type_dir
		          : HasWadExtension(ent->name)    ? &file_type_wad
		                                          : &file_type_file;
		ent->size =
		    stat_ok && ent->type != &file_type_dir ? s.st_size : -1;
		ent->serial_no = dirent->d_ino;
		++*num_entries;
	}

	closedir(dir);

	qsort(*entries, *num_entries, sizeof(struct directory_entry),
	      OrderByName);
	return true;
}

static void RealDirRefresh(void *d, struct directory_entry **entries,
                           size_t *num_entries)
{
	(void) _RealDirRefresh(d, entries, num_entries);
}

static VFILE *RealDirOpen(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	FILE *fs;

	fs = fopen(filename, "r+");
	if (fs == NULL) {
		VFS_StoreError("%s: %s", filename, strerror(errno));
		free(filename);
		return NULL;
	}

	free(filename);
	return vfwrapfile(fs);
}

struct directory *RealDirOpenDir(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *path;
	struct directory *result = NULL;

	if (entry == VFS_PARENT_DIRECTORY) {
		path = PathDirName(dir->path);
		result = VFS_OpenDir(path);
		free(path);
		return result;
	}

	if (entry->type != &file_type_dir && entry->type != &file_type_wad) {
		VFS_StoreError("%s: not a directory", entry->name);
		return NULL;
	}

	path = VFS_EntryPath(dir, entry);
	result = VFS_OpenDir(path);
	free(path);
	return result;
}

static bool RealDirRemove(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	bool result = remove(filename) == 0;
	if (!result) {
		VFS_StoreError("%s: %s", filename, strerror(errno));
	}
	free(filename);
	return result;
}

static bool RealDirRename(void *_dir, struct directory_entry *entry,
                          const char *new_name)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	char *full_new_name = StringJoin("/", dir->path, new_name, NULL);
	bool result = rename(filename, full_new_name) == 0;
	if (!result) {
		VFS_StoreError("%s: %s", filename, strerror(errno));
	}
	free(filename);
	free(full_new_name);
	return result;
}

static const struct directory_funcs realdir_funcs = {
    "file",         // singular
    "files",        // plural
    false,          // ordered
    RealDirRefresh, // refresh
    RealDirOpen,    // open
    RealDirOpenDir, // open_dir
    RealDirRemove,  // remove
    RealDirRename,  // rename
    NULL,           // need_commit
    NULL,           // commit
    NULL,           // swap_entries
    NULL,           // save_snapshot
    NULL,           // restore_snapshot
    NULL,           // free
};

struct directory *VFS_OpenRealDir(const char *path)
{
	struct directory *d = checked_calloc(1, sizeof(struct directory));

	d->directory_funcs = &realdir_funcs;
	VFS_InitDirectory(d, path);
	d->type = &file_type_dir;
	if (!strcmp(path, "/")) {
		free(d->parent_name);
		d->parent_name = NULL;
	}
	if (!_RealDirRefresh(d, &d->entries, &d->num_entries)) {
		VFS_CloseDir(d);
		return NULL;
	}

	return d;
}

VFILE *VFS_Open(const char *path)
{
	VFILE *result = vfwrapfile(fopen(path, "r+"));
	if (result == NULL) {
		VFS_StoreError("%s: %s", path, strerror(errno));
	}
	return result;
}
