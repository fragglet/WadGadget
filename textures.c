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
#include "vfile.h"
#include "vfs.h"

#include "textures.h"

#define TEXTURE_CONFIG_HEADER "; deutex format texture lump configuration\n\n"
#define PNAMES_CONFIG_HEADER "; patch names lump configuration\n\n"

typedef char pname[8];

struct pnames {
	pname *pnames;
	size_t num_pnames;
};

struct patch {
	int16_t originx, originy;
	uint16_t patch;
	uint16_t stepdir;  // unused
	uint16_t colormap;  // unused
};

struct texture {
	char name[8];
	uint32_t masked;  // unused
	uint16_t width, height;
	uint32_t columndirectory;  // unused
	uint16_t patchcount;
	struct patch patches[1];
};

struct textures {
	struct texture **textures;
	size_t num_textures;
	uint64_t *serial_nos;
	struct pnames *pnames;
};

static void FreePnames(struct pnames *pnames)
{
	free(pnames->pnames);
	free(pnames);
}

static struct pnames *ReadPnames(VFILE *f)
{
	struct pnames *pnames = checked_calloc(1, sizeof(struct pnames));
	uint32_t cnt;

	if (vfread(&cnt, sizeof(uint32_t), 1, f) != 1) {
		free(pnames);
		vfclose(f);
		return NULL;
	}

	SwapLE32(&cnt);
	pnames->num_pnames = cnt;

	pnames->pnames = checked_calloc(cnt, sizeof(pname));

	if (vfread(pnames->pnames, sizeof(pname), cnt, f) != cnt) {
		FreePnames(pnames);
		vfclose(f);
		return NULL;
	}

	vfclose(f);
	return pnames;
}

static void SwapTexture(struct texture *t)
{
	int i;

	SwapLE32(&t->masked);
	SwapLE16(&t->width);
	SwapLE16(&t->height);
	SwapLE32(&t->masked);
	SwapLE32(&t->columndirectory);
	SwapLE16(&t->patchcount);

	for (i = 0; i < t->patchcount; i++) {
		SwapLE16((uint16_t *) &t->patches[i].originx);
		SwapLE16((uint16_t *) &t->patches[i].originy);
		SwapLE16(&t->patches[i].patch);
		SwapLE16(&t->patches[i].stepdir);
		SwapLE16(&t->patches[i].colormap);
	}
}

static size_t TextureLen(size_t patchcount)
{
	return sizeof(struct texture)
	     + sizeof(struct patch) * (patchcount - 1);
}

static struct texture *AllocTexture(size_t patchcount)
{
	struct texture *result = checked_calloc(1, TextureLen(patchcount));
	result->patchcount = patchcount;
	return result;
}

/*
static struct texture *DupTexture(struct texture *t)
{
	size_t sz = TextureLen(t->patchcount);
	struct texture *result = checked_malloc(sz);
	memcpy(result, t, sz);
	return result;
}
*/

static struct texture *ReadTexture(const uint8_t *start, size_t max_len)
{
	struct texture *result;
	size_t sz;
	uint16_t patchcount;

	memcpy(&patchcount, start + 20, 2);
	SwapLE16(&patchcount);

	sz = TextureLen(patchcount);
	if (sz > max_len) {
		return NULL;
	}

	result = AllocTexture(patchcount);
	memcpy(result, start, sz);
	SwapTexture(result);

	return result;
}

static void FreeTextures(struct textures *t)
{
	int i;

	for (i = 0; i < t->num_textures; i++) {
		free(t->textures[i]);
	}
	if (t->pnames != NULL) {
		FreePnames(t->pnames);
	}
	free(t->serial_nos);
	free(t);
}

static uint64_t NewSerialNo(void)
{
	static uint64_t counter = 0x800000;
	uint64_t result = counter;
	++counter;
	return result;
}

static void AddSerialNos(struct textures *txs)
{
	int i;

	txs->serial_nos = checked_calloc(txs->num_textures, sizeof(uint64_t));

	for (i = 0; i < txs->num_textures; i++) {
		txs->serial_nos[i] = NewSerialNo();
	}
}

static struct textures *ReadTextures(VFILE *input)
{
	VFILE *sink = vfopenmem(NULL, 0);
	struct textures *result = NULL;
	uint8_t *lump;
	size_t lump_len;
	uint32_t num_textures;
	int i;

	vfcopy(input, sink);
	vfclose(input);

	if (!vfgetbuf(sink, (void **) &lump, &lump_len) || lump_len < 4) {
		goto fail;
	}

	memcpy(&num_textures, lump, sizeof(uint32_t));
	SwapLE32(&num_textures);
	if (lump_len < 4 + 4 * num_textures) {
		goto fail;
	}

	result = checked_calloc(1, sizeof(struct textures));
	result->textures =
		checked_calloc(num_textures, sizeof(struct texture *));
	result->num_textures = num_textures;

	for (i = 0; i < num_textures; i++) {
		uint32_t start;

		memcpy(&start, lump + 4 + 4 * i, sizeof(uint32_t));
		SwapLE32(&start);

		if (start + TextureLen(0) >= lump_len) {
			FreeTextures(result);
			result = NULL;
			goto fail;
		}

		result->textures[i] = ReadTexture(lump + start,
		                                  lump_len - start);
		if (result->textures[i] == NULL) {
			FreeTextures(result);
			result = NULL;
			goto fail;
		}
	}

	AddSerialNos(result);

fail:
	vfclose(sink);
	return result;
}

static VFILE *FormatTextureConfig(struct textures *txs)
{
	struct texture *t;
	struct pnames *pnames = txs->pnames;
	VFILE *result = vfopenmem(NULL, 0);
	char buf[32];
	int i, j;

	assert(vfwrite(TEXTURE_CONFIG_HEADER,
	               strlen(TEXTURE_CONFIG_HEADER), 1, result) == 1);

	for (i = 0; i < txs->num_textures; i++) {
		t = txs->textures[i];

		snprintf(buf, sizeof(buf), "%-8.8s %8d %8d\n", t->name,
		         t->width, t->height);
		assert(vfwrite(buf, strlen(buf), 1, result) == 1);

		for (j = 0; j < t->patchcount; j++) {
			const struct patch *p = &t->patches[j];
			snprintf(buf, sizeof(buf), "* %-8.8s %6d %8d\n",
			         pnames->pnames[p->patch], p->originx,
			         p->originy);
			assert(vfwrite(buf, strlen(buf), 1, result) == 1);
		}
	}

	assert(vfseek(result, 0, SEEK_SET) == 0);

	return result;
}

static bool CheckTextureConfig(struct textures *txs)
{
	int i, j;

	for (i = 0; i < txs->num_textures; i++) {
		struct texture *t = txs->textures[i];

		for (j = 0; j < t->patchcount; j++) {
			if (t->patches[j].patch >= txs->pnames->num_pnames) {
				return false;
			}
		}
	}

	return true;
}

static VFILE *FormatPnamesConfig(struct pnames *p)
{
	VFILE *result = vfopenmem(NULL, 0);
	char buf[32];
	int i;

	assert(vfwrite(PNAMES_CONFIG_HEADER,
	               strlen(PNAMES_CONFIG_HEADER), 1, result) == 1);

	for (i = 0; i < p->num_pnames; i++) {
		snprintf(buf, sizeof(buf), "%.8s\n", p->pnames[i]);
		assert(vfwrite(buf, strlen(buf), 1, result) == 1);
	}

	assert(vfseek(result, 0, SEEK_SET) == 0);

	return result;
}

VFILE *TX_ToTexturesConfig(VFILE *input, VFILE *pnames_input)
{
	struct textures *txs = ReadTextures(input);
	VFILE *result;

	if (txs == NULL) {
		return NULL;
	}

	txs->pnames = ReadPnames(pnames_input);
	if (txs->pnames == NULL || !CheckTextureConfig(txs)) {
		FreeTextures(txs);
		return NULL;
	}

	result = FormatTextureConfig(txs);

	FreeTextures(txs);
	return result;
}

VFILE *TX_ToPnamesConfig(VFILE *input)
{
	struct pnames *p = ReadPnames(input);
	VFILE *result;

	if (p == NULL) {
		return NULL;
	}

	result = FormatPnamesConfig(p);
	FreePnames(p);
	return result;
}

static char *ReadLine(uint8_t *buf, size_t buf_len, unsigned int *offset)
{
	char *result, *p;
	unsigned int start, len;

	if (*offset >= buf_len) {
		return NULL;
	}

	// Skip leading spaces.
	while (*offset < buf_len && buf[*offset] != '\n'
	    && isspace(buf[*offset])) {
		++*offset;
	}

	start = *offset;
	while (*offset < buf_len && buf[*offset] != '\n') {
		++*offset;
	}

	len = *offset - start;
	result = checked_calloc(len + 1, 1);
	memcpy(result, &buf[start], len);
	result[len] = '\0';
	++*offset;

	// Strip out comments.
	p = strchr(result, ';');
	if (p != NULL) {
		*p = '\0';
	}

	return result;
}

static int ScanLine(char *line, const char *fmt, char *name,
                    int *x, int *y)
{
	int nfields;

	*x = 0;
	*y = 0;
	nfields = sscanf(line, fmt, name, x, y);
	if (nfields < 1) {
		return 0;
	}

	// We only allow 8 characters, but parse up to 9 to see if
	// the limit was exceeded.
	name[9] = '\0';
	if (strlen(name) > 8) {
		return 0;
	}

	return nfields;
}

static bool MaybeAddTexture(struct textures *txs, char *line)
{
	struct texture *t;
	char namebuf[10];
	int w, h, n;

	n = ScanLine(line, "%9s %d %d", namebuf, &w, &h);
	if (n < 3) {
		return false;
	}

	t = AllocTexture(0);
	memcpy(t->name, namebuf, 8);
	t->width = w;
	t->height = h;
	t->patchcount = 0;

	txs->textures = checked_realloc(txs->textures,
		(txs->num_textures + 1) * sizeof(struct texture *));
	txs->textures[txs->num_textures] = t;
	++txs->num_textures;

	return true;
}

static bool MaybeAddPatch(struct textures *txs, char *line)
{
	char namebuf[10];
	struct texture *t;
	struct patch *p;
	int x, y, n;

	if (txs->num_textures <= 0) {
		return false;
	}

	n = ScanLine(line, "* %9s %d %d", namebuf, &x, &y);
	if (n < 1) {
		return false;
	}

	// As long as we have the name, we can append the patch
	// (X/Y offsets are assumed to be zero)
	t = txs->textures[txs->num_textures - 1];
	t = checked_realloc(t, TextureLen(t->patchcount + 1));
	txs->textures[txs->num_textures - 1] = t;
	p = &t->patches[t->patchcount];
	++t->patchcount;

	p->originx = x;
	p->originy = y;
	p->patch = 99; // TODO
	p->stepdir = 0;
	p->colormap = 0;

	return true;
}

static struct textures *ParseTextureConfig(uint8_t *buf, size_t buf_len)
{
	struct textures *result;
	unsigned int offset = 0;
	bool fail;

	result = calloc(1, sizeof(struct textures));

	for (;;) {
		char *line = ReadLine(buf, buf_len, &offset);
		if (line == NULL) {
			break;
		}

		fail = strlen(line) > 0
		    && !MaybeAddPatch(result, line)
		    && !MaybeAddTexture(result, line);
		free(line);

		if (fail) {
			// error
			FreeTextures(result);
			return NULL;
		}
	}

	AddSerialNos(result);

	return result;
}

// Implementation of a VFS directory that is backed by a textures list.
// Currently incomplete.
struct texture_dir {
	struct directory dir;
	struct textures *txs;
};

static void FreeEntries(struct directory *d)
{
	int i;

	for (i = 0; i < d->num_entries; i++) {
		free(d->entries[i].name);
	}
	free(d->entries);
	d->entries = NULL;
	d->num_entries = 0;
}

static void TextureDirRefresh(void *_dir)
{
	struct texture_dir *dir = _dir;
	unsigned int i;
	struct directory_entry *ent;

	FreeEntries(&dir->dir);

	dir->dir.num_entries = dir->txs->num_textures;
	dir->dir.entries = checked_calloc(
	dir->txs->num_textures, sizeof(struct directory_entry));

	for (i = 0; i < dir->txs->num_textures; i++) {
		ent = &dir->dir.entries[i];
		ent->type = FILE_TYPE_FILE;  // TODO: _TEXTURE
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

static void TextureDirRemove(void *_dir, struct directory_entry *entry)
{
	struct texture_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.entries;

	if (idx >= dir->txs->num_textures) {
		return;
	}

	free(dir->txs->textures[idx]);
	memmove(&dir->txs->textures[idx], &dir->txs->textures[idx + 1],
	        (dir->dir.num_entries - idx - 1) * sizeof(struct texture *));
	memmove(&dir->txs->serial_nos[idx], &dir->txs->serial_nos[idx + 1],
	        (dir->dir.num_entries - idx - 1) * sizeof(uint64_t));
	--dir->txs->num_textures;
}

static void TextureDirRename(void *_dir, struct directory_entry *entry,
                             const char *new_name)
{
	struct texture_dir *dir = _dir;
	unsigned int idx = entry - dir->dir.entries;

	if (idx >= dir->txs->num_textures) {
		return;
	}

	strncpy(dir->txs->textures[idx]->name, new_name, 8);
}

static void TextureDirCommit(void *dir)
{
	// TODO: write the lump
}

static void TextureDirDescribe(char *buf, size_t buf_len, int cnt)
{
	if (cnt == 1) {
		snprintf(buf, buf_len, "1 texture");
	} else {
		snprintf(buf, buf_len, "%d textures", cnt);
	}
}

static void TextureDirFree(void *_dir)
{
	struct texture_dir *dir = _dir;

	FreeTextures(dir->txs);
}

struct directory_funcs texture_dir_funcs = {
	TextureDirRefresh,
	TextureDirOpen,
	TextureDirRemove,
	TextureDirRename,
	TextureDirCommit,
	TextureDirDescribe,
	TextureDirFree,
};

struct directory *OpenTextureDir(void)
{
	struct texture_dir *dir = calloc(1, sizeof(struct texture_dir));

	dir->dir.type = FILE_TYPE_DIR;  // TODO: texture list
	dir->dir.path = checked_strdup("TEXTURE1");
	dir->dir.refcount = 1;
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.directory_funcs = &texture_dir_funcs;

	// TODO
	dir->txs = ReadTextures(VFS_Open("TEXTURE1.lmp"));

	TextureDirRefresh(dir);

	return &dir->dir;
}
