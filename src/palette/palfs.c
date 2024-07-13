//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "fs/vfs.h"
#include "common.h"
#include "stringlib.h"

#include "palette/palfs.h"

struct palette_dir {
	struct directory dir;
	struct directory *inner;
};

static char *InnerName(const char *name)
{
	return StringJoin("", name, ".png", NULL);
}

static VFILE *PaletteFSOpen(void *dir, struct directory_entry *entry)
{
	struct palette_dir *pd = dir;
	struct directory_entry *inner_ent =
		VFS_EntryBySerial(pd->inner, entry->serial_no);

	if (inner_ent == NULL) {
		return NULL;
	}

	return VFS_OpenByEntry(pd->inner, inner_ent);
}

static struct directory *PaletteFSOpenDir(void *dir,
                                          struct directory_entry *entry)
{
	if (entry == VFS_PARENT_DIRECTORY) {
		// TODO
	}

	return NULL;
}

static void PaletteFSRefresh(void *dir, struct directory_entry **entries,
                             size_t *num_entries)
{
	struct palette_dir *pd = dir;
	int i;

	VFS_Refresh(pd->inner);

	*entries = checked_calloc(pd->inner->num_entries,
	                          sizeof(struct directory_entry));
	*num_entries = 0;

	for (i = 0; i < pd->inner->num_entries; i++) {
		struct directory_entry *inner_ent = &pd->inner->entries[i];
		struct directory_entry *ent = &(*entries)[*num_entries];

		if (inner_ent->type != FILE_TYPE_FILE
		 || !StringHasSuffix(inner_ent->name, ".png")) {
			continue;
		}

		ent->type = FILE_TYPE_FILE;
		ent->name = checked_strdup(inner_ent->name);
		*strstr(ent->name, ".png") = '\0';
		ent->size = -1;
		ent->serial_no = inner_ent->serial_no;

		++*num_entries;
	}
}

static void PaletteFSRemove(void *dir, struct directory_entry *entry)
{
	struct palette_dir *pd = dir;
	struct directory_entry *inner_ent =
		VFS_EntryBySerial(pd->inner, entry->serial_no);

	if (inner_ent == NULL) {
		return;
	}

	VFS_Remove(pd->inner, inner_ent);
}

static void PaletteFSRename(void *dir, struct directory_entry *entry,
                            const char *new_name)
{
	struct palette_dir *pd = dir;
	struct directory_entry *inner_ent =
		VFS_EntryBySerial(pd->inner, entry->serial_no);
	char *full_name;

	if (inner_ent == NULL) {
		return;
	}

	full_name = InnerName(new_name);
	VFS_Rename(pd->inner, inner_ent, full_name);
	free(full_name);
}

static void PaletteFSDescribeEntries(char *buf, size_t buf_len, int cnt)
{
	if (cnt == 1) {
		snprintf(buf, buf_len, "1 palette");
	} else {
		snprintf(buf, buf_len, "%d palettes", cnt);
	}
}

static void PaletteFSFree(void *dir)
{
	struct palette_dir *pd = dir;

	VFS_CloseDir(pd->inner);
}

static const struct directory_funcs palette_fs_functions = {
	PaletteFSRefresh,
	PaletteFSOpen,
	PaletteFSOpenDir,
	PaletteFSRemove,
	PaletteFSRename,
	NULL, // need_commit
	NULL, // commit
	PaletteFSDescribeEntries,
	NULL, // swap_entries
	NULL, // save_snapshot
	NULL, // restore_snapshot
	PaletteFSFree,
};

static const char *GetPalettesPath(void)
{
	static char *result;
	const char *home;

	if (result != NULL) {
		return result;
	}

	home = getenv("HOME");
	assert(home != NULL);

	result = MakeDirectories(home, ".config", "WadGadget",
	                         "Palettes", NULL);
	assert(result != NULL);
	return result;
}

struct directory *PAL_OpenDirectory(void)
{
	struct palette_dir *pd = checked_calloc(1, sizeof(struct palette_dir));
	const char *path = GetPalettesPath();
	struct directory *inner;

	inner = VFS_OpenDir(path);
	if (inner == NULL) {
		return NULL;
	}

	pd->dir.directory_funcs = &palette_fs_functions;
	VFS_InitDirectory(&pd->dir, path);
	pd->dir.type = FILE_TYPE_PALETTES;
	pd->inner = inner;

	PaletteFSRefresh(pd, &pd->dir.entries, &pd->dir.num_entries);

	return &pd->dir;
}
