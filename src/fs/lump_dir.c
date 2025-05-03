//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// Common code for filesystem implementations based on editing the contents
// of WAD lumps.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "fs/lump_dir.h"
#include "stringlib.h"
#include "ui/title_bar.h"

struct directory *VFS_LumpDirGetParent(struct directory *_dir,
                                       struct directory_entry **ent)
{
	struct lump_based_dir *dir = (struct lump_based_dir *) _dir;

	if (ent != NULL) {
		*ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
		assert(*ent != NULL);
	}

	return dir->parent_dir;
}

static bool LoadFromLump(struct lump_based_dir *dir,
                         struct directory_entry *ent)
{
	VFILE *in;

	// We always allow the user to create a new directory by making an
	// empty lump and opening it.
	if (ent->size == 0) {
		dir->funcs->init_empty(dir);
		dir->loaded = true;
	} else {
		in = VFS_OpenByEntry(dir->parent_dir, ent);
		dir->loaded = dir->funcs->unmarshal(dir, in, 0);
	}

	dir->last_commit = dir->funcs->modified_count(dir);

	return dir->loaded;
}

static void SaveToLump(struct lump_based_dir *dir)
{
	struct wad_file *wf = VFS_WadFile(dir->parent_dir);
	struct directory_entry *ent;
	VFILE *out, *marshaled;

	// No changes to save?
	if (dir->last_commit == 0) {
		return;
	}

	ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
	if (ent == NULL) {
		return;
	}

	out = W_OpenLumpRewrite(wf, ent - dir->parent_dir->entries);
	if (out == NULL) {
		return;
	}

	marshaled = dir->funcs->marshal(dir);
	assert(marshaled != NULL);

	vfcopy(marshaled, out);
	vfclose(marshaled);
	vfclose(out);

	VFS_CommitChanges(dir->parent_dir, "update of '%s'", ent->name);
	UI_ShowNotice("%s lump updated.", ent->name);
}

struct directory *VFS_LumpDirOpenDir(void *_dir, struct directory_entry *ent)
{
	struct lump_based_dir *dir = _dir;

	if (ent == VFS_PARENT_DIRECTORY) {
		VFS_DirectoryRef(dir->parent_dir);
		return dir->parent_dir;
	}

	return NULL;
}

void VFS_LumpDirFree(void *_dir)
{
	struct lump_based_dir *dir = (struct lump_based_dir *) _dir;
	if (dir->loaded) {
		SaveToLump(dir);
	}
	VFS_DirectoryUnref(dir->parent_dir);
}

bool VFS_LumpDirNeedCommit(void *_dir)
{
	struct lump_based_dir *dir = (struct lump_based_dir *) _dir;

	return dir->funcs->modified_count(dir) > dir->last_commit;
}

void VFS_LumpDirCommit(void *_dir)
{
	struct lump_based_dir *dir = (struct lump_based_dir *) _dir;
	dir->last_commit = dir->funcs->modified_count(dir);
}

VFILE *VFS_LumpDirSaveSnapshot(void *_dir)
{
	struct lump_based_dir *dir = (struct lump_based_dir *) _dir;
	VFILE *tmp, *result = vfopenmem(NULL, 0);
	int modcount = dir->funcs->modified_count(dir);

	assert(vfwrite(&modcount, sizeof(int), 1, result) == 1);

	tmp = dir->funcs->marshal(dir);
	vfcopy(tmp, result);
	vfclose(tmp);

	vfseek(result, 0, SEEK_SET);
	return result;
}

void VFS_LumpDirRestoreSnapshot(void *_dir, VFILE *in)
{
	struct lump_based_dir *dir = (struct lump_based_dir *) _dir;
	int modcount;

	assert(vfread(&modcount, sizeof(int), 1, in) == 1);
	assert(dir->funcs->unmarshal(dir, in, modcount));
	dir->last_commit = modcount;
}

bool VFS_LumpDirInit(struct lump_based_dir *dir,
                     const struct lump_based_dir_funcs *funcs,
                     struct directory *parent, struct directory_entry *ent)
{
	struct directory_revision *rev;

	dir->dir.path =
	    StringJoin(DIR_SEPARATOR_S, parent->path, ent->name, NULL);
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.readonly = parent->readonly;
	dir->dir.parent_name =
	    StringJoin("", "Back to ", PathBaseName(parent->path), NULL);

	dir->loaded = false;
	dir->funcs = funcs;
	dir->parent_dir = parent;
	dir->lump_serial = ent->serial_no;
	VFS_DirectoryRef(&dir->dir);
	VFS_DirectoryRef(dir->parent_dir);

	if (!LoadFromLump(dir, ent)) {
		VFS_CloseDir(&dir->dir);
		return false;
	}

	rev = VFS_SaveRevision(&dir->dir);
	snprintf(rev->descr, VFS_REVISION_DESCR_LEN, "Initial version");

	VFS_Refresh(&dir->dir);

	return true;
}
