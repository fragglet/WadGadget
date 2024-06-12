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
#include "ui/ui.h"

#include "textures/textures.h"
#include "textures/internal.h"

struct pnames_dir {
	struct lump_dir dir;

	// pn->modified_count at the last call to commit.
	unsigned int last_commit;
};

#define PNAMES(d) ((d)->dir.b.pn)

static void PnamesDirRefresh(void *_dir, struct directory_entry **entries,
                             size_t *num_entries)
{
	struct pnames_dir *dir = _dir;
	struct directory_entry *new_entries;
	int i;

	*num_entries = PNAMES(dir)->num_pnames;
	new_entries = checked_calloc(*num_entries,
	                             sizeof(struct directory_entry));
	for (i = 0; i < *num_entries; i++) {
		new_entries[i].name = checked_calloc(9, 1);
		memcpy(new_entries[i].name, PNAMES(dir)->pnames[i], 8);
		new_entries[i].name[8] = '\0';

		new_entries[i].type = FILE_TYPE_PNAME;
		new_entries[i].size = 0;
		new_entries[i].serial_no =
			TX_PnameSerialNo(PNAMES(dir)->pnames[i]);
	}

	*entries = new_entries;
}

static void PnamesDirRemove(void *_dir, struct directory_entry *entry)
{
	struct pnames_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.dir.entries;

	assert(idx < PNAMES(dir)->num_pnames);
	TX_RemovePname(PNAMES(dir), idx);
}

static void PnamesDirRename(void *_dir, struct directory_entry *entry,
                            const char *new_name)
{
	struct pnames_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.dir.entries;

	assert(idx < PNAMES(dir)->num_pnames);
	TX_RenamePname(PNAMES(dir), idx, new_name);
}

static bool PnamesDirNeedCommit(void *_dir)
{
	struct pnames_dir *dir = _dir;

	return PNAMES(dir)->modified_count > dir->last_commit;
}

static void PnamesDirCommit(void *_dir)
{
	struct pnames_dir *dir = _dir;

	dir->last_commit = PNAMES(dir)->modified_count;
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

	assert(x < PNAMES(dir)->num_pnames);
	assert(y < PNAMES(dir)->num_pnames);

	memcpy(tmp, PNAMES(dir)->pnames[x], 8);
	memcpy(PNAMES(dir)->pnames[x], PNAMES(dir)->pnames[y], 8);
	memcpy(PNAMES(dir)->pnames[y], tmp, 8);

	++PNAMES(dir)->modified_count;
}

static VFILE *PnamesDirSaveSnapshot(void *_dir)
{
	struct pnames_dir *dir = _dir;
	VFILE *tmp, *result = vfopenmem(NULL, 0);

	assert(vfwrite(&PNAMES(dir)->modified_count,
	               sizeof(int), 1, result) == 1);

	tmp = TX_MarshalPnames(PNAMES(dir));

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

	TX_FreePnames(PNAMES(dir));
	PNAMES(dir) = pn;
	dir->last_commit = mod_count;
}

static void PnamesDirFree(void *_dir)
{
	struct pnames_dir *dir = _dir;

	TX_LumpDirFree(&dir->dir);
	TX_FreePnames(PNAMES(dir));
}

static const struct directory_funcs pnames_dir_funcs = {
	PnamesDirRefresh,
	NULL,  // open
	TX_LumpDirOpenDir,
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

static bool PnamesDirLoad(void *_dir, struct directory *wad_dir,
                          struct directory_entry *ent)
{
	struct pnames_dir *dir = _dir;
	VFILE *in = VFS_OpenByEntry(wad_dir, ent);
	return TX_BundleLoadPnames(&dir->dir.b, in);
}

static bool PnamesDirSave(void *_dir, struct directory *wad_dir,
                          struct directory_entry *ent)
{
	struct pnames_dir *dir = _dir;
	struct wad_file *wf = VFS_WadFile(wad_dir);
	VFILE *out, *marshaled;

	if (PNAMES(dir)->modified_count == 0) {
		return true;
	}

	marshaled = TX_MarshalPnames(PNAMES(dir));
	assert(marshaled != NULL);

	out = W_OpenLumpRewrite(wf, ent - wad_dir->entries);
	if (out == NULL) {
		vfclose(marshaled);
		return false;
	}

	vfcopy(marshaled, out);
	vfclose(marshaled);
	vfclose(out);

	VFS_CommitChanges(wad_dir, "update of '%s'", ent->name);
	UI_ShowNotice("%s lump updated.", ent->name);

	return true;
}

static struct pnames *MakeSubset(struct pnames *pn, struct file_set *selected)
{
	struct pnames *result = checked_calloc(1, sizeof(struct pnames));
	int i;

	for (i = 0; i < pn->num_pnames; i++) {
		if (VFS_SetHas(selected, TX_PnameSerialNo(pn->pnames[i]))) {
			TX_AppendPname(result, pn->pnames[i]);
		}
	}

	pn->modified_count = 0;
	return result;
}

static VFILE *PnamesDirFormatConfig(void *_dir, struct file_set *selected)
{
	struct pnames_dir *dir = _dir;
	struct pnames *subset;
	VFILE *result;

	if (selected != NULL) {
		subset = MakeSubset(PNAMES(dir), selected);
	} else {
		subset = PNAMES(dir);
	}
	result = TX_FormatPnamesConfig(subset);

	if (subset != PNAMES(dir)) {
		TX_FreePnames(subset);
	}

	return result;
}

static const struct lump_dir_funcs pnames_lump_dir_funcs = {
	PnamesDirLoad,
	PnamesDirSave,
	PnamesDirFormatConfig,
	TX_BundleParsePnames,
};

struct directory *TX_OpenPnamesDir(struct directory *parent,
                                   struct directory_entry *ent)
{
	struct pnames_dir *dir =
		checked_calloc(1, sizeof(struct pnames_dir));

	dir->dir.dir.type = FILE_TYPE_PNAMES_LIST;
	dir->dir.dir.directory_funcs = &pnames_dir_funcs;
	if (!TX_InitLumpDir(&dir->dir, &pnames_lump_dir_funcs,  parent, ent)) {
		return NULL;
	}

	return &dir->dir.dir;
}
