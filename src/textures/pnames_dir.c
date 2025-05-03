//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "fs/lump_dir.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "fs/wad_file.h"
#include "textures/internal.h"
#include "textures/textures.h"
#include "ui/title_bar.h"

struct pnames_dir {
	struct lump_based_dir dir;
	struct pnames *pn;
};

const struct file_type file_type_pnames_list = {"Patch names list"};
const struct file_type file_type_pname = {"Patch name"};

static void PnamesDirRefresh(void *_dir, struct directory_entry **entries,
                             size_t *num_entries)
{
	struct pnames_dir *dir = _dir;
	struct directory_entry *new_entries;
	int i;

	*num_entries = dir->pn->num_pnames;
	new_entries =
	    checked_calloc(*num_entries, sizeof(struct directory_entry));
	for (i = 0; i < *num_entries; i++) {
		new_entries[i].name = checked_calloc(9, 1);
		memcpy(new_entries[i].name, dir->pn->pnames[i], 8);
		new_entries[i].name[8] = '\0';

		new_entries[i].type = &file_type_pname;
		new_entries[i].size = 0;
		new_entries[i].serial_no = TX_PnameSerialNo(dir->pn->pnames[i]);
	}

	*entries = new_entries;
}

static bool PnamesDirRemove(void *_dir, struct directory_entry *entry)
{
	struct pnames_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.dir.entries;

	assert(idx < dir->pn->num_pnames);
	TX_RemovePname(dir->pn, idx);
	// TODO: Change asserts to failure result
	return true;
}

static bool PnamesDirRename(void *_dir, struct directory_entry *entry,
                            const char *new_name)
{
	struct pnames_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.dir.entries;

	assert(idx < dir->pn->num_pnames);
	TX_RenamePname(dir->pn, idx, new_name);
	// TODO: Change asserts to failure result
	return true;
}

static void PnamesDirSwapEntries(void *_dir, unsigned int x, unsigned int y)
{
	struct pnames_dir *dir = _dir;
	pname tmp;

	assert(x < dir->pn->num_pnames);
	assert(y < dir->pn->num_pnames);

	memcpy(tmp, dir->pn->pnames[x], 8);
	memcpy(dir->pn->pnames[x], dir->pn->pnames[y], 8);
	memcpy(dir->pn->pnames[y], tmp, 8);

	++dir->pn->modified_count;
}

static void PnamesDirFree(void *_dir)
{
	struct pnames_dir *dir = _dir;

	TX_FreePnames(dir->pn);
	VFS_LumpDirFree(&dir->dir);
}

static const struct directory_funcs pnames_dir_funcs = {
    "pname",
    "pnames",
    true,
    PnamesDirRefresh,
    NULL, // open
    VFS_LumpDirOpenDir,
    PnamesDirRemove,
    PnamesDirRename,
    VFS_LumpDirNeedCommit,
    VFS_LumpDirCommit,
    PnamesDirSwapEntries,
    VFS_LumpDirSaveSnapshot,
    VFS_LumpDirRestoreSnapshot,
    PnamesDirFree,
};

static void PnamesDirInitEmpty(void *_dir)
{
	struct pnames_dir *dir = _dir;

	UI_ShowNotice("Creating a new, empty PNAMES directory.");
	dir->pn = TX_NewPnamesList(0);
	dir->pn->modified_count = 1;
}

static VFILE *PnamesDirMarshal(void *_dir)
{
	struct pnames_dir *dir = _dir;
	return TX_MarshalPnames(dir->pn);
}

static bool PnamesDirUnmarshal(void *_dir, VFILE *in, int mod_count)
{
	struct pnames_dir *dir = _dir;
	if (dir->pn != NULL) {
		TX_FreePnames(dir->pn);
	}
	dir->pn = TX_UnmarshalPnames(in);
	if (dir->pn == NULL) {
		return false;
	}
	dir->pn->modified_count = mod_count;
	return true;
}

static int PnamesDirModCount(void *_dir)
{
	struct pnames_dir *dir = _dir;
	return dir->pn->modified_count;
}

static const struct lump_based_dir_funcs pnames_lump_dir_funcs = {
    PnamesDirInitEmpty,
    PnamesDirMarshal,
    PnamesDirUnmarshal,
    PnamesDirModCount,
};

struct directory *TX_OpenPnamesDir(struct directory *parent,
                                   struct directory_entry *ent)
{
	struct pnames_dir *dir = checked_calloc(1, sizeof(struct pnames_dir));

	dir->dir.dir.type = &file_type_pnames_list;
	dir->dir.dir.directory_funcs = &pnames_dir_funcs;
	if (!VFS_LumpDirInit(&dir->dir, &pnames_lump_dir_funcs, parent, ent)) {
		free(dir);
		return NULL;
	}

	return &dir->dir.dir;
}

struct pnames *TX_PnamesList(struct directory *_dir)
{
	struct pnames_dir *dir = (struct pnames_dir *) _dir;
	assert(dir->dir.dir.directory_funcs == &pnames_dir_funcs);
	return dir->pn;
}
