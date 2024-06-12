//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// Common code shared between texture and pnames directories.

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "stringlib.h"

#include "textures/textures.h"
#include "textures/internal.h"

struct directory *TX_LumpDirOpenDir(void *_dir, struct directory_entry *ent)
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
	struct directory_entry *ent;

	TX_DirGetParent(&dir->dir, &ent);
	dir->lump_dir_funcs->save(dir, dir->parent_dir, ent);
	TX_FreeBundle(&dir->b);
	VFS_DirectoryUnref(TX_DirGetParent(&dir->dir, NULL));
}

bool TX_DirReload(struct directory *_dir)
{
	struct lump_dir *dir = (struct lump_dir *) _dir;
	struct directory_entry *ent;
	int txs_modcount, pn_modcount;
	bool first, success;

	TX_DirGetParent(_dir, &ent);

	first = dir->b.txs == NULL;
	if (!first) {
		txs_modcount = dir->b.txs->modified_count,
		pn_modcount = dir->b.pn->modified_count;
	}
	success = dir->lump_dir_funcs->load(dir, dir->parent_dir, ent);
	VFS_Refresh(&dir->dir);

	if (success && !first) {
		dir->b.txs->modified_count = txs_modcount + 1;
		dir->b.pn->modified_count = pn_modcount + 1;
	}

	return success;
}

bool TX_DirSave(struct directory *_dir)
{
	struct lump_dir *dir = (struct lump_dir *) _dir;
	struct directory_entry *ent;
	bool result;

	TX_DirGetParent(_dir, &ent);

	result = dir->lump_dir_funcs->save(dir, dir->parent_dir, ent);
	VFS_Refresh(dir->parent_dir);

	return result;
}

struct texture_bundle *TX_DirGetBundle(struct directory *_dir)
{
	struct lump_dir *dir = (struct lump_dir *) _dir;

	return &dir->b;
}

VFILE *TX_DirFormatConfig(struct directory *_dir, struct file_set *subset)
{
	struct lump_dir *dir = (struct lump_dir *) _dir;

	return dir->lump_dir_funcs->format_config(dir, subset);
}

bool TX_DirParseConfig(struct directory *_dir, struct texture_bundle *b,
                       VFILE *in)
{
	struct lump_dir *dir = (struct lump_dir *) _dir;

	return dir->lump_dir_funcs->parse_config(b, in);
}

bool TX_InitLumpDir(struct lump_dir *dir, const struct lump_dir_funcs *funcs,
                    struct directory *parent, struct directory_entry *ent)
{
	struct directory_revision *rev;

	dir->dir.path = StringJoin("/", parent->path, ent->name, NULL);
	dir->dir.refcount = 1;
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.readonly = parent->readonly;

	dir->lump_dir_funcs = funcs;
	dir->parent_dir = parent;
	VFS_DirectoryRef(dir->parent_dir);
	dir->lump_serial = ent->serial_no;

	if (!TX_DirReload(&dir->dir)) {
		VFS_CloseDir(&dir->dir);
		return false;
	}

	rev = VFS_SaveRevision(&dir->dir);
	snprintf(rev->descr, VFS_REVISION_DESCR_LEN, "Initial version");

	return true;

}
