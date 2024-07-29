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

struct wad_directory {
	struct directory dir;
	struct wad_file *wad_file;
};

static void WadDirectoryRefresh(void *_dir, struct directory_entry **entries,
                                size_t *num_entries)
{
	struct wad_directory *dir = _dir;
	struct wad_file_entry *waddir = W_GetDirectory(dir->wad_file);
	unsigned int i, num_lumps = W_NumLumps(dir->wad_file);

	*entries = checked_calloc(num_lumps, sizeof(struct directory_entry));

	for (i = 0; i < num_lumps; i++) {
		struct directory_entry *ent = *entries + i;
		ent->type = FILE_TYPE_LUMP;
		ent->name = checked_calloc(9, 1);
		memcpy(ent->name, waddir[i].name, 8);
		ent->name[8] = '\0';
		ent->size = waddir[i].size;
		ent->serial_no = waddir[i].serial_no;
	}

	*num_entries = num_lumps;
}

static VFILE *WadDirOpen(void *_dir, struct directory_entry *entry)
{
	struct wad_directory *dir = _dir;
	// TODO: We shoud ideally do something that will more reliably
	// map back to the WAD file lump number after inserting and
	// deleting lumps:
	unsigned int lump_index = entry - dir->dir.entries;

	return W_OpenLump(dir->wad_file, lump_index);
}

struct directory *WadDirOpenDir(void *_dir, struct directory_entry *entry)
{
	struct wad_directory *dir = _dir;

	if (entry == VFS_PARENT_DIRECTORY) {
		char *path = PathDirName(dir->dir.path);
		struct directory *result = VFS_OpenDir(path);
		free(path);
		return result;
	} else {
		VFS_StoreError("%s: not a directory", entry->name);
		return NULL;
	}
}

static bool WadDirRemove(void *_dir, struct directory_entry *entry)
{
	struct wad_directory *dir = _dir;
	W_DeleteEntry(dir->wad_file, entry - dir->dir.entries);
	// TODO: The asserts in W_DeleteEntries() can be reworked to allow a
	// failure return value here.
	return true;
}

static bool WadDirRename(void *_dir, struct directory_entry *entry,
                         const char *new_name)
{
	struct wad_directory *dir = _dir;
	// TODO: Check new name is valid?
	W_SetLumpName(dir->wad_file, entry - dir->dir.entries, new_name);
	// TODO: The asserts in W_SetLumpName() can be reworked to allow a
	// failure return value here.
	return true;
}

static void WadDirCommit(void *_dir)
{
	struct wad_directory *dir = _dir;
	W_CommitChanges(dir->wad_file);
}

static void WadDirFree(void *_dir)
{
	struct wad_directory *dir = _dir;
	if (dir->wad_file != NULL) {
		W_CloseFile(dir->wad_file);
	}
}

static void WadDirSwapEntries(void *_dir, unsigned int x, unsigned int y)
{
	struct wad_directory *dir = _dir;
	W_SwapEntries(dir->wad_file, x, y);
}

static bool WadDirNeedCommit(void *_dir)
{
	struct wad_directory *dir = _dir;
	return W_NeedCommit(dir->wad_file);
}

static VFILE *WadDirSaveSnapshot(void *_dir)
{
	struct wad_directory *dir = _dir;
	return W_SaveSnapshot(dir->wad_file);
}

static void WadDirRestoreSnapshot(void *_dir, VFILE *in)
{
	struct wad_directory *dir = _dir;
	return W_RestoreSnapshot(dir->wad_file, in);
}

static const struct directory_funcs waddir_funcs = {
	"lump", "lumps",
	WadDirectoryRefresh,
	WadDirOpen,
	WadDirOpenDir,
	WadDirRemove,
	WadDirRename,
	WadDirNeedCommit,
	WadDirCommit,
	WadDirSwapEntries,
	WadDirSaveSnapshot,
	WadDirRestoreSnapshot,
	WadDirFree,
};

struct directory *VFS_OpenWadAsDirectory(const char *path)
{
	struct directory_revision *rev;
	struct wad_directory *d =
		checked_calloc(1, sizeof(struct wad_directory));

	d->dir.directory_funcs = &waddir_funcs;
	VFS_InitDirectory(&d->dir, path);
	d->dir.type = FILE_TYPE_WAD;
	d->wad_file = W_OpenFile(path);
	if (d->wad_file == NULL) {
		VFS_CloseDir(&d->dir);
		return NULL;
	}
	// The directory is read-only if the file is read-only, but
	// also if the file is an IWAD (we require confirmation first)
	d->dir.readonly = W_IsReadOnly(d->wad_file) || W_IsIWAD(d->wad_file);
	WadDirectoryRefresh(d, &d->dir.entries, &d->dir.num_entries);
	rev = VFS_SaveRevision(&d->dir);
	snprintf(rev->descr, VFS_REVISION_DESCR_LEN, "Initial version");

	return &d->dir;
}

struct wad_file *VFS_WadFile(struct directory *dir)
{
	struct wad_directory *wdir;

	if (dir->type != FILE_TYPE_WAD) {
		return NULL;
	}

	wdir = (struct wad_directory *) dir;
	return wdir->wad_file;
}
