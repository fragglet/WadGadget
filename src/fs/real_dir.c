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

struct real_directory {
	struct directory d;
	char **serials;
	size_t num_serials;
};

const struct file_type file_type_dir = {"Directory"};
const struct file_type file_type_file = {"File"};

static int HasWadExtension(const char *name)
{
	const char *extn;
	if (strlen(name) < 4) {
		return 0;
	}
	extn = name + strlen(name) - 4;
	return !strcasecmp(extn, ".wad") || !strcasecmp(extn, ".rts");
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

static int SerialCompare(const void *a, const void *b)
{
	return strcmp(*((const char **) a), *((const char **) b));
}

static int SerialByName(struct real_directory *d, const char *name)
{
	int min, max, i, cmp;

	min = 0;
	max = d->num_serials;

	while (min < max) {
		i = (min + max) / 2;
		cmp = strcmp(name, d->serials[i]);
		if (cmp == 0) {
			return i;
		} else if (cmp < 0) {
			max = i;
		} else {
			min = i + 1;
		}
	}

	return -1;
}

// Assign serial numbers to each directory entry. Serial numbers must remain
// consistent when the directory is refreshed. To do this, we keep a sorted
// array of filenames (d->serials) and use the memory address of the filename
// string as the serial number.
static void AssignSerials(struct real_directory *d,
                          struct directory_entry *entries, size_t num_entries)
{
	size_t new_sz, new_num_serials;
	int i, s;

	new_sz = (d->num_serials + num_entries) * sizeof(char *);
	d->serials = checked_realloc(d->serials, new_sz);
	new_num_serials = d->num_serials;

	for (i = 0; i < num_entries; ++i) {
		s = SerialByName(d, entries[i].name);
		if (s < 0) {
			s = new_num_serials;
			d->serials[s] = checked_strdup(entries[i].name);
			++new_num_serials;
		}
		entries[i].serial_no = (long) d->serials[s];
	}

	qsort(d->serials, new_num_serials, sizeof(char *), SerialCompare);
	d->num_serials = new_num_serials;
}

static bool _RealDirRefresh(struct real_directory *d,
                            struct directory_entry **entries,
                            size_t *num_entries)
{
	DIR *dir;

	*entries = NULL;
	*num_entries = 0;

	dir = opendir(d->d.path);
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
		path = StringJoin(DIR_SEPARATOR_S, d->d.path, dirent->d_name,
		                  NULL);
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
		++*num_entries;
	}

	closedir(dir);

	qsort(*entries, *num_entries, sizeof(struct directory_entry),
	      OrderByName);

	AssignSerials(d, *entries, *num_entries);
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

	fs = fopen(filename, "rb+");
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
	char *full_new_name =
	    StringJoin(DIR_SEPARATOR_S, dir->path, new_name, NULL);
	bool result = rename(filename, full_new_name) == 0;
	if (!result) {
		VFS_StoreError("%s: %s", filename, strerror(errno));
	}
	free(filename);
	free(full_new_name);
	return result;
}

static void RealDirFree(void *_dir)
{
	struct real_directory *dir = _dir;
	int i;

	for (i = 0; i < dir->num_serials; ++i) {
		free(dir->serials[i]);
	}
	free(dir->serials);
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
    RealDirFree,    // free
};

struct directory *VFS_OpenRealDir(const char *path)
{
	struct real_directory *d =
	    checked_calloc(1, sizeof(struct real_directory));

	d->d.directory_funcs = &realdir_funcs;
	VFS_InitDirectory(&d->d, path);
	d->d.type = &file_type_dir;
	if (!strcmp(path, "/")) { // unix root
		free(d->d.parent_name);
		d->d.parent_name = NULL;
	}
	if (!_RealDirRefresh(d, &d->d.entries, &d->d.num_entries)) {
		VFS_CloseDir(&d->d);
		return NULL;
	}

	return &d->d;
}

VFILE *VFS_Open(const char *path)
{
	VFILE *result = vfwrapfile(fopen(path, "rb+"));
	if (result == NULL) {
		VFS_StoreError("%s: %s", path, strerror(errno));
	}
	return result;
}
