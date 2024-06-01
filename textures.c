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
#include <string.h>
#include <assert.h>

#include "common.h"
#include "vfile.h"

#include "textures.h"

#define TEXTURE_CONFIG_HEADER "; deutex format texture lump configuration\n\n"
#define PNAMES_CONFIG_HEADER "; patch names lump configuration\n\n"

typedef char pname[8];

struct pnames {
	pname *pnames;
	size_t num_pnames;
};

struct patch {
	uint16_t originx, originy;
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
		SwapLE16(&t->patches[i].originx);
		SwapLE16(&t->patches[i].originy);
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
	struct texture *result = checked_malloc(TextureLen(patchcount));
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
	free(t);
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
