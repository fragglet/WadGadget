//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "stringlib.h"
#include "fs/vfs.h"
#include "fs/wad_file.h"

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
	int cmp = (dy->type == FILE_TYPE_DIR)
	        - (dx->type == FILE_TYPE_DIR);
	if (cmp != 0) {
		return cmp;
	}
	return strcasecmp(dx->name, dy->name);
}

static bool _RealDirRefresh(struct directory *d)
{
	DIR *dir;

	VFS_FreeEntries(d);

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
		stat_ok = stat(path, &s) == 0;
		// For symlinks, follow the link to get the actual file type.
		// This ensures links to directories are handled properly.
		if (stat_ok && dirent->d_type == DT_LNK) {
			dirent->d_type = S_ISDIR(s.st_mode) ? DT_DIR : DT_REG;
		}
		free(path);
		path = checked_strdup(dirent->d_name);

		d->entries = checked_realloc(d->entries,
			sizeof(struct directory_entry) * (d->num_entries + 1));
		ent = &d->entries[d->num_entries];
		ent->name = path;
		ent->type = dirent->d_type == DT_DIR ? FILE_TYPE_DIR :
		            HasWadExtension(ent->name) ? FILE_TYPE_WAD :
		            FILE_TYPE_FILE;
		ent->size = stat_ok
		         && ent->type != FILE_TYPE_DIR ? s.st_size : -1;
		ent->serial_no = dirent->d_ino;
		++d->num_entries;
	}

	closedir(dir);

	qsort(d->entries, d->num_entries, sizeof(struct directory_entry),
	      OrderByName);
	return true;
}

static void RealDirRefresh(void *d)
{
	(void) _RealDirRefresh(d);
}

static VFILE *RealDirOpen(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	FILE *fs;

	fs = fopen(filename, "r+");
	free(filename);

	if (fs == NULL) {
		return NULL;
	}

	return vfwrapfile(fs);
}

static void RealDirRemove(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	remove(filename);
	free(filename);
}

static void RealDirRename(void *_dir, struct directory_entry *entry,
                          const char *new_name)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	char *full_new_name = StringJoin("/", dir->path, new_name, NULL);
	rename(filename, full_new_name);
	free(filename);
	free(full_new_name);
}

static void RealDirDescribeEntries(char *buf, size_t buf_len, int cnt)
{
	if (cnt == 1) {
		snprintf(buf, buf_len, "1 file");
	} else {
		snprintf(buf, buf_len, "%d files", cnt);
	}
}

static const struct directory_funcs realdir_funcs = {
	RealDirRefresh,
	RealDirOpen,
	RealDirRemove,
	RealDirRename,
	NULL,
	RealDirDescribeEntries,
	NULL,
};
struct directory *VFS_OpenDir(const char *path)
{
	struct directory *d;

	// TODO: This is kind of a hack.
	if (HasWadExtension(path)) {
		return VFS_OpenWadAsDirectory(path);
	}

	d = checked_calloc(1, sizeof(struct directory));

	d->directory_funcs = &realdir_funcs;
	VFS_InitDirectory(d, path);
	d->type = FILE_TYPE_DIR;
	if (!_RealDirRefresh(d)) {
		VFS_CloseDir(d);
		return NULL;
	}

	return d;
}

struct directory *VFS_OpenDirByEntry(struct directory *dir,
                                     struct directory_entry *entry)
{
	char *path = VFS_EntryPath(dir, entry);
	struct directory *result = NULL;

	switch (entry->type) {
	case FILE_TYPE_DIR:
		result = VFS_OpenDir(path);
		break;

	case FILE_TYPE_WAD:
		result = VFS_OpenWadAsDirectory(path);
		break;

	default:
		break;
	}

	free(path);
	return result;
}
