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

// Yes, Si units, not binary ones.
#define KB(x) (x * 1000ULL)
#define MB(x) (KB(x) * 1000ULL)
#define GB(x) (MB(x) * 1000ULL)
#define TB(x) (GB(x) * 1000ULL)

struct directory_entry _vfs_parent_directory = {
	FILE_TYPE_DIR, "..",
};

char *VFS_EntryPath(struct directory *dir, struct directory_entry *entry)
{
	return StringJoin("/", dir->path, entry->name, NULL);
}

void VFS_FreeEntries(struct directory *d)
{
	int i;

	for (i = 0; i < d->num_entries; i++) {
		free(d->entries[i].name);
	}
	free(d->entries);
	d->entries = NULL;
	d->num_entries = 0;
}

void VFS_InitDirectory(struct directory *d, const char *path)
{
	d->path = PathSanitize(path);
	d->refcount = 1;
	d->entries = NULL;
	d->num_entries = 0;
}

VFILE *VFS_Open(const char *path)
{
	FILE *fs;

	fs = fopen(path, "r+");
	if (fs == NULL) {
		return NULL;
	}

	return vfwrapfile(fs);
}

VFILE *VFS_OpenByEntry(struct directory *dir, struct directory_entry *entry)
{
	return dir->directory_funcs->open(dir, entry);
}

struct directory *VFS_OpenDirByEntry(struct directory *dir,
                                     struct directory_entry *entry)
{
	return dir->directory_funcs->open_dir(dir, entry);
}

struct directory_entry *VFS_EntryBySerial(struct directory *dir,
                                          uint64_t serial_no)
{
	int i;

	for (i = 0; i < dir->num_entries; i++) {
		if (dir->entries[i].serial_no == serial_no) {
			return &dir->entries[i];
		}
	}

	return NULL;
}

struct directory_entry *VFS_EntryByName(struct directory *dir,
                                        const char *name)
{
	int i;

	for (i = 0; i < dir->num_entries; i++) {
		if (!strcmp(dir->entries[i].name, name)) {
			return &dir->entries[i];
		}
	}

	return NULL;
}

void VFS_CommitChanges(struct directory *dir)
{
	if (dir->directory_funcs->commit != NULL) {
		dir->directory_funcs->commit(dir);
	}
}

void VFS_Refresh(struct directory *dir)
{
	dir->directory_funcs->refresh(dir);
}

void VFS_Remove(struct directory *dir, struct directory_entry *entry)
{
	unsigned int index = entry - dir->entries;

	dir->directory_funcs->remove(dir, entry);

	memmove(&dir->entries[index], &dir->entries[index + 1],
	        (dir->num_entries - index - 1)
	          * sizeof(struct directory_entry));
	--dir->num_entries;
}

void VFS_Rename(struct directory *dir, struct directory_entry *entry,
                const char *new_name)
{
	dir->directory_funcs->rename(dir, entry, new_name);
}

void VFS_DirectoryRef(struct directory *dir)
{
	++dir->refcount;
}

void VFS_DirectoryUnref(struct directory *dir)
{
	--dir->refcount;

	if (dir->refcount == 0) {
		if (dir->directory_funcs->free != NULL) {
			dir->directory_funcs->free(dir);
		}
		VFS_FreeEntries(dir);
		free(dir->path);
		free(dir);
	}
}

void VFS_DescribeSize(const struct directory_entry *ent, char buf[10],
                      bool shorter)
{
	int64_t adj_len = ent->size * (shorter ? 100 : 1);

	if (adj_len < 0) {
		strncpy(buf, "", 10);
	} else if (adj_len < KB(100)) {  // up to 99999
		snprintf(buf, 10, "%d", (int) ent->size);
	} else if (adj_len < MB(10)) {  // up to 9999K
		snprintf(buf, 10, "%dK", (short) (ent->size / KB(1)));
	} else if (adj_len < GB(10)) {  // up to 9999M
		snprintf(buf, 10, "%dM", (short) (ent->size / MB(1)));
	} else if (adj_len < TB(10)) {  // up to 9999G
		snprintf(buf, 10, "%dG", (short) (ent->size / GB(1)));
	} else {
		snprintf(buf, 10, "big!");
	}
}

bool VFS_SwapEntries(struct directory *dir, unsigned int x, unsigned int y)
{
	struct directory_entry tmp;
	if (dir->directory_funcs->swap_entries == NULL) {
		return false;
	}
	dir->directory_funcs->swap_entries(dir, x, y);
	tmp = dir->entries[x];
	dir->entries[x] = dir->entries[y];
	dir->entries[y] = tmp;
	return true;
}

#ifdef TEST
int main(int argc, char *argv[])
{
	struct directory *dir;
	int i;

	dir = VFS_OpenDir(argv[1]);

	for (i = 0; i < dir->num_entries; i++) {
		printf("%4d%8d %s\n", dir->entries[i].type,
		       dir->entries[i].size, dir->entries[i].name);
	}

	return 0;
}
#endif
