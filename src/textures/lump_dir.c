//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "stringlib.h"

#include "textures/internal.h"
#include "textures/textures.h"

struct directory *TX_LumpDirOpenDir(void *_dir,
                                    struct directory_entry *ent)
{
	struct lump_dir *dir = _dir;

	if (ent == VFS_PARENT_DIRECTORY) {
		VFS_DirectoryRef(dir->parent_dir);
		return dir->parent_dir;
	}

	return NULL;
}

struct directory *TX_DirGetParent(struct directory *_dir,
                                  struct directory_entry **ent)
{
	struct lump_dir *dir = (struct lump_dir *) _dir;

	if (ent != NULL) {
		*ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
		assert(*ent != NULL);
	}

	return dir->parent_dir;
}

void TX_LumpDirFree(struct lump_dir *dir)
{
	VFS_DirectoryUnref(TX_DirGetParent(&dir->dir, NULL));
}

struct pnames *TX_GetDirPnames(struct directory *dir)
{
	struct pnames *result = TX_PnamesDirGetPnames(dir);
	if (result == NULL) {
		result = TX_TexturesDirGetPnames(dir);
		assert(result != NULL);
	}
	return result;
}

void TX_InitLumpDir(struct lump_dir *dir, struct directory *parent,
                    struct directory_entry *ent)
{
	dir->dir.path = StringJoin("/", parent->path, ent->name, NULL);
	dir->dir.refcount = 1;
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.readonly = parent->readonly;

	dir->parent_dir = parent;
	VFS_DirectoryRef(dir->parent_dir);
	dir->lump_serial = ent->serial_no;
}
