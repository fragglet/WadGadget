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
#include "textures/textures.h"

struct pnames_dir {
	struct directory dir;
	struct pnames *pn;

	// Parent directory; always a WAD file.
	struct directory *parent_dir;
	uint64_t lump_serial;

	// txs->modified_count at the last call to commit.
	unsigned int last_commit;
};

static uint64_t PnameSerialNo(char *pname)
{
	uint64_t result = 0;
	int i;

	for (i = 0; i < 8 && pname[i] != '\0'; i++) {
		result <<= 8;
		result |= pname[i];
	}

	return result;
}

static void PnamesDirRefresh(void *_dir, struct directory_entry **entries,
                             size_t *num_entries)
{
	struct pnames_dir *dir = _dir;
	struct directory_entry *new_entries;
	int i;

	*num_entries = dir->pn->num_pnames;
	new_entries = checked_calloc(*num_entries,
	                             sizeof(struct directory_entry));
	for (i = 0; i < *num_entries; i++) {
		new_entries[i].name = checked_calloc(9, 1);
		memcpy(new_entries[i].name, dir->pn->pnames[i], 8);
		new_entries[i].name[8] = '\0';

		new_entries[i].type = FILE_TYPE_FILE;  // TODO
		new_entries[i].size = 0;
		new_entries[i].serial_no = PnameSerialNo(dir->pn->pnames[i]);
	}

	*entries = new_entries;
}

static struct directory *PnamesDirOpenDir(void *_dir,
                                          struct directory_entry *ent)
{
	struct pnames_dir *dir = _dir;

	if (ent == VFS_PARENT_DIRECTORY) {
		VFS_DirectoryRef(dir->parent_dir);
		return dir->parent_dir;
	}

	return NULL;
}

static void PnamesDirRemove(void *_dir, struct directory_entry *entry)
{
	struct pnames_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.entries;

	assert(idx < dir->pn->num_pnames);
	TX_RemovePname(dir->pn, idx);
}

static void PnamesDirRename(void *_dir, struct directory_entry *entry,
                            const char *new_name)
{
	struct pnames_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.entries;

	assert(idx < dir->pn->num_pnames);
	TX_RenamePname(dir->pn, idx, new_name);
}

static bool PnamesDirNeedCommit(void *_dir)
{
	struct pnames_dir *dir = _dir;

	return dir->pn->modified_count > dir->last_commit;
}

static void PnamesDirCommit(void *_dir)
{
	struct pnames_dir *dir = _dir;

	dir->last_commit = dir->pn->modified_count;
}

static void PnamesDirDescribeEntries(char *buf, size_t buf_len, int cnt)
{
	if (cnt == 1) {
		snprintf(buf, buf_len, "1 patch name");
	} else {
		snprintf(buf, buf_len, "%d patch names", cnt);
	}
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

static VFILE *PnamesDirSaveSnapshot(void *_dir)
{
	struct pnames_dir *dir = _dir;
	VFILE *tmp, *result = vfopenmem(NULL, 0);

	assert(vfwrite(&dir->pn->modified_count,
	               sizeof(int), 1, result) == 1);

	tmp = TX_MarshalPnames(dir->pn);

	vfcopy(tmp, result);
	vfclose(tmp);
	vfseek(result, 0, SEEK_SET);
	return result;
}

static void PnamesDirRestoreSnapshot(void *_dir, VFILE *in)
{
	struct pnames_dir *dir = _dir;
	struct pnames *pn;
	unsigned int mod_count;

	assert(vfread(&mod_count, sizeof(int), 1, in) == 1);
	pn = TX_UnmarshalPnames(in);
	assert(pn != NULL);
	pn->modified_count = mod_count;

	TX_FreePnames(dir->pn);
	dir->pn = pn;
	dir->last_commit = mod_count;
}

static void PnamesDirFree(void *_dir)
{
	struct pnames_dir *dir = _dir;

	if (dir->last_commit > 0) {
		// TODO: Save new directory
	}

	TX_FreePnames(dir->pn);
	VFS_DirectoryUnref(dir->parent_dir);
}

static struct directory_funcs pnames_dir_funcs = {
	PnamesDirRefresh,
	NULL,  // open
	PnamesDirOpenDir,
	PnamesDirRemove,
	PnamesDirRename,
	PnamesDirNeedCommit,
	PnamesDirCommit,
	PnamesDirDescribeEntries,
	PnamesDirSwapEntries,
	PnamesDirSaveSnapshot,
	PnamesDirRestoreSnapshot,
	PnamesDirFree,
};

static bool LoadPnamesDir(struct pnames_dir *dir)
{
	struct directory_entry *ent;
	VFILE *input;

	ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
	if (ent == NULL) {
		return false;
	}

	input = VFS_OpenByEntry(dir->parent_dir, ent);
	if (input == NULL) {
		return false;
	}

	dir->pn = TX_UnmarshalPnames(input);
	if (dir->pn == NULL) {
		return false;
	}

	return true;
}

struct directory *TX_OpenPnamesDir(struct directory *parent,
                                   struct directory_entry *ent)
{
	struct directory_revision *rev;
	struct pnames_dir *dir =
		checked_calloc(1, sizeof(struct pnames_dir));

	dir->dir.type = FILE_TYPE_DIR;  // TODO
	dir->dir.path = StringJoin("/", parent->path, ent->name, NULL);
	dir->dir.refcount = 1;
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.directory_funcs = &pnames_dir_funcs;
	dir->dir.readonly = parent->readonly;

	dir->parent_dir = parent;
	VFS_DirectoryRef(dir->parent_dir);
	dir->lump_serial = ent->serial_no;

	if (!LoadPnamesDir(dir)) {
		PnamesDirFree(dir);
		return NULL;
	}

	rev = VFS_SaveRevision(&dir->dir);
	snprintf(rev->descr, VFS_REVISION_DESCR_LEN, "Initial version");

	PnamesDirRefresh(dir, &dir->dir.entries, &dir->dir.num_entries);

	return &dir->dir;
}
