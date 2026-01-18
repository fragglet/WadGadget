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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs/lump_dir.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "textures/textures.h"
#include "ui/title_bar.h"

// Implementation of a VFS directory that is backed by a textures list.
struct texture_dir {
	struct lump_based_dir dir;

	struct texture_bundle b;
};

#define TEXTURES(dir) ((dir)->b.txs)
#define PNAMES(dir)   ((dir)->b.pn)

const struct file_type file_type_texture_list = {"Textures"};
const struct file_type file_type_texture = {"Texture"};

static void TextureDirRefresh(void *_dir, struct directory_entry **entries,
                              size_t *num_entries)
{
	struct texture_dir *dir = _dir;
	unsigned int i;

	*num_entries = TEXTURES(dir)->num_textures;
	*entries = checked_calloc(TEXTURES(dir)->num_textures,
	                          sizeof(struct directory_entry));

	for (i = 0; i < TEXTURES(dir)->num_textures; i++) {
		struct directory_entry *ent = *entries + i;
		ent->type = &file_type_texture;
		ent->name = checked_calloc(9, 1);
		memcpy(ent->name, TEXTURES(dir)->textures[i]->name, 8);
		ent->name[8] = '\0';
		ent->size = 0;
		ent->serial_no = TEXTURES(dir)->serial_nos[i];
	}
}

static VFILE *TextureDirOpen(void *dir, struct directory_entry *entry)
{
	return NULL;
}

static bool TextureDirRemove(void *_dir, struct directory_entry *entry)
{
	struct texture_dir *dir = _dir;

	TX_RemoveTexture(TEXTURES(dir), entry - dir->dir.dir.entries);
	// TODO: Change asserts to failure result
	return true;
}

static bool TextureDirRename(void *_dir, struct directory_entry *entry,
                             const char *new_name)
{
	struct texture_dir *dir = _dir;

	// TODO: Set an error message
	return TX_RenameTexture(TEXTURES(dir), entry - dir->dir.dir.entries,
	                        new_name);
}

static void TextureDirSwap(void *_dir, unsigned int x, unsigned int y)
{
	struct texture_dir *dir = _dir;
	struct texture *tmp;
	uint64_t tmp_serial;

	assert(x < TEXTURES(dir)->num_textures);
	assert(y < TEXTURES(dir)->num_textures);

	tmp = TEXTURES(dir)->textures[x];
	TEXTURES(dir)->textures[x] = TEXTURES(dir)->textures[y];
	TEXTURES(dir)->textures[y] = tmp;

	tmp_serial = TEXTURES(dir)->serial_nos[x];
	TEXTURES(dir)->serial_nos[x] = TEXTURES(dir)->serial_nos[y];
	TEXTURES(dir)->serial_nos[y] = tmp_serial;

	++TEXTURES(dir)->modified_count;
}

static void TextureDirFree(void *_dir)
{
	struct texture_dir *dir = _dir;
	TX_BundleSavePnamesTo(&dir->b, dir->dir.parent_dir);
	VFS_LumpDirFree(&dir->dir);
	TX_FreePnames(PNAMES(dir));
}

struct directory_funcs texture_dir_funcs = {
    "texture",
    "textures",
    true,
    TextureDirRefresh,
    TextureDirOpen,
    VFS_LumpDirOpenDir,
    TextureDirRemove,
    TextureDirRename,
    VFS_LumpDirNeedCommit,
    VFS_LumpDirCommit,
    TextureDirSwap,
    VFS_LumpDirSaveSnapshot,
    VFS_LumpDirRestoreSnapshot,
    TextureDirFree,
};

static void TextureDirInitEmpty(void *_dir)
{
	struct texture_dir *dir = _dir;

	UI_ShowNotice("Creating a new, empty texture directory.");

	dir->b.txs = TX_NewTextureList(0);
	// TODO: For TEXTURE1 we should probably create an
	// AASTINKY-style dummy texture as the first entry.
	++dir->b.txs->modified_count;
}

static VFILE *TextureDirMarshal(void *_dir)
{
	struct texture_dir *dir = _dir;
	return TX_MarshalTextures(TEXTURES(dir));
}

static bool TextureDirUnmarshal(void *_dir, VFILE *in, int mod_count)
{
	struct texture_dir *dir = _dir;
	if (TEXTURES(dir) != NULL) {
		TX_FreeTextures(TEXTURES(dir));
	}
	TEXTURES(dir) = TX_UnmarshalTextures(in);
	if (TEXTURES(dir) == NULL) {
		return false;
	}
	TEXTURES(dir)->modified_count = mod_count;
	return true;
}

static int TextureDirModCount(void *_dir)
{
	struct texture_dir *dir = _dir;
	return TEXTURES(dir)->modified_count;
}

static const struct lump_based_dir_funcs texture_lump_dir_funcs = {
    TextureDirInitEmpty,
    TextureDirMarshal,
    TextureDirUnmarshal,
    TextureDirModCount,
};

struct directory *TX_OpenTextureDir(struct directory *parent,
                                    struct directory_entry *ent)
{
	struct texture_dir *dir = checked_calloc(1, sizeof(struct texture_dir));

	if (!TX_BundleLoadPnamesFrom(&dir->b, parent)) {
		free(dir);
		return NULL;
	}

	dir->dir.dir.type = &file_type_texture_list;
	dir->dir.dir.directory_funcs = &texture_dir_funcs;
	if (!VFS_LumpDirInit(&dir->dir, &texture_lump_dir_funcs, parent, ent)) {
		TX_FreePnames(PNAMES(dir));
		free(dir);
		return NULL;
	}

	return &dir->dir.dir;
}

struct texture_bundle *TX_DirGetBundle(struct directory *_dir)
{
	struct texture_dir *dir = (struct texture_dir *) _dir;
	assert(dir->dir.dir.directory_funcs == &texture_dir_funcs);
	return &dir->b;
}

struct textures *TX_TextureList(struct directory *dir)
{
	return TX_DirGetBundle(dir)->txs;
}
