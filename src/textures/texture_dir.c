//
// Copyright(C) 2024 Simon Howard
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
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "conv/error.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "stringlib.h"
#include "ui/ui.h"

#include "textures/textures.h"

// Implementation of a VFS directory that is backed by a textures list.
// Currently incomplete.
struct texture_dir {
	struct directory dir;
	struct textures *txs;
	struct pnames *pn;

	// Parent directory; always a WAD file.
	struct directory *parent_dir;
	uint64_t lump_serial;

	// txs->modified_count at the last call to commit.
	unsigned int last_commit;
};

static void TextureDirRefresh(void *_dir)
{
	struct texture_dir *dir = _dir;
	unsigned int i;
	struct directory_entry *ent;

	VFS_FreeEntries(&dir->dir);

	dir->dir.num_entries = dir->txs->num_textures;
	dir->dir.entries = checked_calloc(
	dir->txs->num_textures, sizeof(struct directory_entry));

	for (i = 0; i < dir->txs->num_textures; i++) {
		ent = &dir->dir.entries[i];
		ent->type = FILE_TYPE_TEXTURE;
		ent->name = checked_calloc(9, 1);
		memcpy(ent->name, dir->txs->textures[i]->name, 8);
		ent->name[8] = '\0';
		ent->size = 0;
		ent->serial_no = dir->txs->serial_nos[i];
	}
}

static VFILE *TextureDirOpen(void *dir, struct directory_entry *entry)
{
	return NULL;
}

static struct directory *TextureDirOpenDir(void *_dir,
                                           struct directory_entry *ent)
{
	struct texture_dir *dir = _dir;

	if (ent == VFS_PARENT_DIRECTORY) {
		VFS_DirectoryRef(dir->parent_dir);
		return dir->parent_dir;
	}

	return NULL;
}

static void TextureDirRemove(void *_dir, struct directory_entry *entry)
{
	struct texture_dir *dir = _dir;

	TX_RemoveTexture(dir->txs, entry - dir->dir.entries);
}

static void TextureDirRename(void *_dir, struct directory_entry *entry,
                             const char *new_name)
{
	struct texture_dir *dir = _dir;

	TX_RenameTexture(dir->txs, entry - dir->dir.entries, new_name);
}

static bool TextureDirNeedCommit(void *_dir)
{
	struct texture_dir *dir = _dir;

	return dir->txs->modified_count > dir->last_commit;
}

static void TextureDirCommit(void *_dir)
{
	struct texture_dir *dir = _dir;

	dir->last_commit = dir->txs->modified_count;
}

static void TextureDirDescribe(char *buf, size_t buf_len, int cnt)
{
	if (cnt == 1) {
		snprintf(buf, buf_len, "1 texture");
	} else {
		snprintf(buf, buf_len, "%d textures", cnt);
	}
}

static bool TextureDirSave(struct texture_dir *dir)
{
	struct directory_entry *ent;
	struct wad_file *wf;
	VFILE *out, *texture_out;
	unsigned int idx;

	// Unchanged since it was opened?
	if (dir->txs->modified_count == 0) {
		return true;
	}

	ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
	if (ent == NULL) {
		return false;
	}

	texture_out = TX_MarshalTextures(dir->txs);
	if (texture_out == NULL) {
		return false;
	}

	wf = VFS_WadFile(dir->parent_dir);
	assert(wf != NULL);
	idx = ent - dir->parent_dir->entries;
	out = W_OpenLumpRewrite(wf, idx);
	if (out == NULL) {
		vfclose(texture_out);
		return false;
	}

	vfcopy(texture_out, out);
	vfclose(texture_out);
	vfclose(out);
	VFS_CommitChanges(dir->parent_dir, "update of '%s' texture directory",
	                  ent->name);
	VFS_Refresh(dir->parent_dir);

	UI_ShowNotice("%s lump updated.", ent->name);

	return true;
}

static void TextureDirSwap(void *_dir, unsigned int x, unsigned int y)
{
	struct texture_dir *dir = _dir;
	struct texture *tmp;
	uint64_t tmp_serial;

	assert(x < dir->txs->num_textures);
	assert(y < dir->txs->num_textures);

	tmp = dir->txs->textures[x];
	dir->txs->textures[x] = dir->txs->textures[y];
	dir->txs->textures[y] = tmp;

	tmp_serial = dir->txs->serial_nos[x];
	dir->txs->serial_nos[x] = dir->txs->serial_nos[y];
	dir->txs->serial_nos[y] = tmp_serial;

	++dir->txs->modified_count;
}

static VFILE *TextureDirSaveSnapshot(void *_dir)
{
	struct texture_dir *dir = _dir;
	VFILE *tmp, *result = vfopenmem(NULL, 0);

	assert(vfwrite(&dir->txs->modified_count,
	               sizeof(int), 1, result) == 1);

	tmp = TX_MarshalTextures(dir->txs);
	vfcopy(tmp, result);
	vfclose(tmp);
	vfseek(result, 0, SEEK_SET);
	return result;
}

static void TextureDirRestoreSnapshot(void *_dir, VFILE *in)
{
	struct texture_dir *dir = _dir;
	struct textures *new_txs;
	unsigned int mod_count;

	assert(vfread(&mod_count, sizeof(int), 1, in) == 1);
	new_txs = TX_UnmarshalTextures(in);
	new_txs->modified_count = mod_count;

	TX_FreeTextures(dir->txs);
	dir->txs = new_txs;
	dir->last_commit = mod_count;
}

static void TextureDirFree(void *_dir)
{
	struct texture_dir *dir = _dir;

	if (dir->txs != NULL) {
		TextureDirSave(dir);
		TX_FreeTextures(dir->txs);
	}
	if (dir->pn != NULL) {
		TX_FreePnames(dir->pn);
	}
	VFS_DirectoryUnref(dir->parent_dir);
}

struct directory_funcs texture_dir_funcs = {
	TextureDirRefresh,
	TextureDirOpen,
	TextureDirOpenDir,
	TextureDirRemove,
	TextureDirRename,
	TextureDirNeedCommit,
	TextureDirCommit,
	TextureDirDescribe,
	TextureDirSwap,
	TextureDirSaveSnapshot,
	TextureDirRestoreSnapshot,
	TextureDirFree,
};

struct directory *TX_DirGetParent(struct directory *_dir,
                                  struct directory_entry **ent)
{
	struct texture_dir *dir = (struct texture_dir *) _dir;

	assert(dir->dir.directory_funcs == &texture_dir_funcs);

	if (ent != NULL) {
		*ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
		assert(*ent != NULL);
	}

	return dir->parent_dir;
}

struct pnames *TX_GetDirPnames(struct directory *_dir)
{
	struct texture_dir *dir = (struct texture_dir *) _dir;

	assert(dir->dir.directory_funcs == &texture_dir_funcs);

	return dir->pn;
}

static struct pnames *LoadPnames(struct texture_dir *dir)
{
	VFILE *input;
	struct pnames *pn;
	struct directory_entry *ent =
		VFS_EntryByName(dir->parent_dir, "PNAMES");

	if (ent == NULL) {
		ConversionError("WAD does not contain a PNAMES lump.");
		return NULL;
	}

	input = VFS_OpenByEntry(dir->parent_dir, ent);
	if (input == NULL) {
		ConversionError("Failed to open PNAMES lump.");
		return NULL;
	}

	pn = TX_UnmarshalPnames(input);
	if (pn == NULL) {
		ConversionError("Failed to unmarshal PNAMES");
		return NULL;
	}

	return pn;
}

bool TX_DirReload(struct directory *_dir)
{
	struct texture_dir *dir = (struct texture_dir *) _dir;
	struct directory *parent;
	struct directory_entry *ent;
	struct textures *new_txs;
	struct pnames *new_pn;
	VFILE *input;

	parent = TX_DirGetParent(_dir, &ent);

	new_pn = LoadPnames(dir);
	if (new_pn == NULL) {
		return false;
	}

	input = VFS_OpenByEntry(parent, ent);
	if (input == NULL) {
		TX_FreePnames(new_pn);
		return false;
	}

	new_txs = TX_UnmarshalTextures(input);
	if (new_txs == NULL) {
		TX_FreePnames(new_pn);
		return false;
	}

	if (dir->pn != NULL) {
		TX_FreePnames(dir->pn);
	}
	if (dir->txs != NULL) {
		TX_FreeTextures(dir->txs);
	}
	dir->pn = new_pn;
	dir->txs = new_txs;
	TextureDirRefresh(dir);
	return true;
}

struct directory *TX_OpenTextureDir(struct directory *parent,
                                    struct directory_entry *ent)
{
	struct directory_revision *rev;
	struct texture_dir *dir =
		checked_calloc(1, sizeof(struct texture_dir));

	dir->dir.type = FILE_TYPE_TEXTURE_LIST;
	dir->dir.path = StringJoin("/", parent->path, ent->name, NULL);
	dir->dir.refcount = 1;
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.directory_funcs = &texture_dir_funcs;

	dir->parent_dir = parent;
	VFS_DirectoryRef(dir->parent_dir);
	dir->lump_serial = ent->serial_no;

	if (!TX_DirReload(&dir->dir)) {
		TextureDirFree(dir);
		return NULL;
	}

	rev = VFS_SaveRevision(&dir->dir);
	snprintf(rev->descr, VFS_REVISION_DESCR_LEN, "Initial version");

	return &dir->dir;
}

struct textures *TX_TextureList(struct directory *_dir)
{
	struct texture_dir *dir = (struct texture_dir *) _dir;

	assert(dir->dir.directory_funcs == &texture_dir_funcs);

	return dir->txs;
}
