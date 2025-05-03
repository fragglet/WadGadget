//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef FS__LUMP_DIR_H_INCLUDED
#define FS__LUMP_DIR_H_INCLUDED

#include "fs/vfile.h"
#include "fs/vfs.h"

struct lump_based_dir_funcs {
	VFILE *(*marshal)(void *_dir);
	bool (*unmarshal)(void *_dir, VFILE *in, int mod_count);
	int (*modified_count)(void *_dir);
};

// TODO: Rename to lump_dir once textures/lump_dir.c is gone.
struct lump_based_dir {
	struct directory dir;
	const struct lump_based_dir_funcs *funcs;

	// Parent directory; always a WAD file.
	struct directory *parent_dir;
	uint64_t lump_serial;

	bool loaded;
	int last_commit; // modified count at last commit time
};

struct directory *VFS_LumpDirGetParent(struct directory *_dir,
                                       struct directory_entry **ent);
struct directory *VFS_LumpDirOpenDir(void *_dir, struct directory_entry *ent);
void VFS_LumpDirFree(void *_dir);
bool VFS_LumpDirNeedCommit(void *_dir);
void VFS_LumpDirCommit(void *_dir);
VFILE *VFS_LumpDirSaveSnapshot(void *_dir);
void VFS_LumpDirRestoreSnapshot(void *_dir, VFILE *in);
bool VFS_LumpDirInit(struct lump_based_dir *dir,
                     const struct lump_based_dir_funcs *funcs,
                     struct directory *parent, struct directory_entry *ent);

#endif /* #ifndef FS__LUMP_DIR_H_INCLUDED */
