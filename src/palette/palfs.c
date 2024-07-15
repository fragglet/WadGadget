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

#include "palette/palette.h"
#include "palette/palfs.h"

#include "ui/dialog.h"

struct palette_dir {
	struct directory dir;
	// The inner directory is the "backing" directory that stores the
	// collection of palette files in the user's home dir.
	struct directory *inner;
	// The previous directory is the directory that the user will
	// return to when exiting the palette view.
	struct directory *previous;
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
	struct palette_dir *pd = dir;

	if (entry == VFS_PARENT_DIRECTORY) {
		VFS_DirectoryRef(pd->previous);
		return pd->previous;
	}

	return NULL;
}

static void PaletteFSRefresh(void *dir, struct directory_entry **entries,
                             size_t *num_entries)
{
	struct palette_dir *pd = dir;
	char *def_pal = PAL_ReadDefaultPointer();
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
		if (!strcmp(inner_ent->name, def_pal)) {
			char *old_name = ent->name;
			ent->name = StringJoin("", old_name,
			                       " [default]", NULL);
			free(old_name);
		}
		ent->size = -1;
		ent->serial_no = inner_ent->serial_no;

		++*num_entries;
	}
}

static bool PaletteFSRemove(void *dir, struct directory_entry *entry)
{
	struct palette_dir *pd = dir;
	struct directory_entry *inner_ent =
		VFS_EntryBySerial(pd->inner, entry->serial_no);
	char *def_pal;
	bool is_default;

	if (inner_ent == NULL) {
		VFS_StoreError("%s: can't find inner entry", entry->name);
		return false;
	}

	def_pal = PAL_ReadDefaultPointer();
	is_default = !strcmp(def_pal, inner_ent->name);
	free(def_pal);

	if (is_default) {
		VFS_StoreError("You can't delete the default palette.");
		return false;
	}

	return VFS_Remove(pd->inner, inner_ent);
}

static bool PaletteFSRename(void *dir, struct directory_entry *entry,
                            const char *new_name)
{
	struct palette_dir *pd = dir;
	struct directory_entry *inner_ent =
		VFS_EntryBySerial(pd->inner, entry->serial_no);
	char *full_name, *def_pal;
	bool is_default, success;

	if (inner_ent == NULL) {
		VFS_StoreError("%s: can't find inner entry", entry->name);
		return false;
	}

	def_pal = PAL_ReadDefaultPointer();
	is_default = !strcmp(def_pal, inner_ent->name);
	free(def_pal);
	full_name = InnerName(new_name);
	success = VFS_Rename(pd->inner, inner_ent, full_name);

	if (is_default && success) {
		PAL_SetDefaultPointer(full_name);
	}

	free(full_name);
	return success;
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
	VFS_DirectoryUnref(pd->previous);
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

struct directory *PAL_OpenDirectory(struct directory *previous)
{
	struct palette_dir *pd = checked_calloc(1, sizeof(struct palette_dir));
	const char *path = PAL_GetPalettesPath();
	struct directory *inner;

	inner = VFS_OpenDir(path);
	if (inner == NULL) {
		return NULL;
	}

	pd->dir.directory_funcs = &palette_fs_functions;
	VFS_InitDirectory(&pd->dir, path);
	pd->dir.type = FILE_TYPE_PALETTES;
	free(pd->dir.parent_name);
	pd->dir.parent_name = StringJoin("", "Back to ",
	                                 PathBaseName(previous->path), NULL);
	pd->inner = inner;
	pd->previous = previous;
	VFS_DirectoryRef(pd->previous);

	PaletteFSRefresh(pd, &pd->dir.entries, &pd->dir.num_entries);

	return &pd->dir;
}
