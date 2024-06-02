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
#include "fs/vfile.h"
#include "fs/vfs.h"

#include "textures/textures.h"

void TX_FreePnames(struct pnames *pnames)
{
	free(pnames->pnames);
	free(pnames);
}

struct pnames *TX_UnmarshalPnames(VFILE *f)
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
		TX_FreePnames(pnames);
		vfclose(f);
		return NULL;
	}

	vfclose(f);
	return pnames;
}

VFILE *TX_MarshalPnames(struct pnames *pn)
{
	VFILE *result = vfopenmem(NULL, 0);
	uint32_t cnt = pn->num_pnames;

	SwapLE32(&cnt);
	if (vfwrite(&cnt, sizeof(uint32_t), 1, result) != 1) {
		vfclose(result);
		return NULL;
	}

	if (vfwrite(pn->pnames, sizeof(pname),
	            pn->num_pnames, result) != pn->num_pnames) {
		vfclose(result);
		return NULL;
	}
	vfseek(result, 0, SEEK_SET);
	return result;
}

int TX_GetPnameIndex(struct pnames *pn, const char *name)
{
	int i;

	for (i = 0; i < pn->num_pnames; i++) {
		if (!strncasecmp(pn->pnames[i], name, 8)) {
			return i;
		}
	}

	return -1;
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

struct texture *TX_AllocTexture(size_t patchcount)
{
	struct texture *result = checked_calloc(1, TextureLen(patchcount));
	result->patchcount = patchcount;
	return result;
}

struct texture *TX_DupTexture(struct texture *t)
{
	size_t sz = TextureLen(t->patchcount);
	struct texture *result = checked_malloc(sz);
	memcpy(result, t, sz);
	return result;
}

static struct texture *UnmarshalTexture(const uint8_t *start, size_t max_len)
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

	result = TX_AllocTexture(patchcount);
	memcpy(result, start, sz);
	SwapTexture(result);

	return result;
}

void TX_FreeTextures(struct textures *t)
{
	int i;

	for (i = 0; i < t->num_textures; i++) {
		free(t->textures[i]);
	}
	if (t->pnames != NULL) {
		TX_FreePnames(t->pnames);
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

void TX_AddSerialNos(struct textures *txs)
{
	int i;

	txs->serial_nos = checked_calloc(txs->num_textures, sizeof(uint64_t));

	for (i = 0; i < txs->num_textures; i++) {
		txs->serial_nos[i] = NewSerialNo();
	}
}

struct textures *TX_UnmarshalTextures(VFILE *input)
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
			TX_FreeTextures(result);
			result = NULL;
			goto fail;
		}

		result->textures[i] =
			UnmarshalTexture(lump + start, lump_len - start);
		if (result->textures[i] == NULL) {
			TX_FreeTextures(result);
			result = NULL;
			goto fail;
		}
	}

	TX_AddSerialNos(result);

fail:
	vfclose(sink);
	return result;
}

VFILE *TX_MarshalTextures(struct textures *txs)
{
	VFILE *result = vfopenmem(NULL, 0);
	uint32_t *offsets =
		checked_calloc(txs->num_textures, sizeof(uint32_t));
	uint32_t num_textures = txs->num_textures;
	size_t lump_len = 4 + 4 * txs->num_textures;
	int i;

	for (i = 0; i < txs->num_textures; i++) {
		offsets[i] = lump_len;
		SwapLE32(&offsets[i]);
		lump_len += TextureLen(txs->textures[i]->patchcount);
	}

	SwapLE32(&num_textures);
	assert(vfwrite(&num_textures, sizeof(uint32_t), 1, result) == 1);
	assert(vfwrite(offsets, sizeof(uint32_t),
	               txs->num_textures, result) == txs->num_textures);

	for (i = 0; i < txs->num_textures; i++) {
		struct texture *swapped = TX_DupTexture(txs->textures[i]);
		size_t len = TextureLen(txs->textures[i]->patchcount);
		SwapTexture(swapped);
		assert(vfwrite(swapped, 1, len, result) == len);
		free(swapped);
	}

	vfseek(result, 0, SEEK_SET);
	return result;
}

struct texture *TX_AddPatch(struct texture *t, struct patch *p)
{
	struct patch *newp;

	// As long as we have the name, we can append the patch
	// (X/Y offsets are assumed to be zero)
	t = checked_realloc(t, TextureLen(t->patchcount + 1));
	newp = &t->patches[t->patchcount];
	++t->patchcount;

	*newp = *p;

	return t;
}
