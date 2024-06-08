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

#include "textures/textures.h"

void TX_FreePnames(struct pnames *pnames)
{
	free(pnames->pnames);
	free(pnames);
}

struct pnames *TX_UnmarshalPnames(VFILE *f)
{
	struct pnames *pnames = checked_calloc(1, sizeof(struct pnames));
	uint32_t cnt, names_read;

	if (vfread(&cnt, sizeof(uint32_t), 1, f) != 1) {
		ConversionError("Failed to read 4 byte lump header.");
		free(pnames);
		vfclose(f);
		return NULL;
	}

	SwapLE32(&cnt);
	pnames->num_pnames = cnt;
	pnames->modified = false;
	pnames->pnames = checked_calloc(cnt, sizeof(pname));

	names_read = vfread(pnames->pnames, sizeof(pname), cnt, f);
	if (names_read != cnt) {
		ConversionError("Lump too short: only %d/%d names read.",
		                names_read, cnt);
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
		ConversionError("Failed to write header.");
		vfclose(result);
		return NULL;
	}

	if (vfwrite(pn->pnames, sizeof(pname),
	            pn->num_pnames, result) != pn->num_pnames) {
		ConversionError("Failed to write names.");
		vfclose(result);
		return NULL;
	}
	vfseek(result, 0, SEEK_SET);
	return result;
}

int TX_AppendPname(struct pnames *pn, const char *name)
{
	int result;

	pn->pnames = checked_realloc(pn->pnames,
	                             sizeof(pname) * (pn->num_pnames + 1));
	strncpy(pn->pnames[pn->num_pnames], name, 8);
	result = pn->num_pnames;
	++pn->num_pnames;
	pn->modified = true;
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
	SwapLE32(&t->masked);
	SwapLE16(&t->width);
	SwapLE16(&t->height);
	SwapLE32(&t->masked);
	SwapLE32(&t->columndirectory);
	SwapLE16(&t->patchcount);
}

static void SwapTexturePatches(struct texture *t)
{
	int i;

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
		ConversionError("Texture length %d exceeds maximum length %d",
		                (int) sz, (int) max_len);
		return NULL;
	}

	result = TX_AllocTexture(patchcount);
	memcpy(result, start, sz);
	SwapTexture(result);
	SwapTexturePatches(result);

	return result;
}

void TX_FreeTextures(struct textures *t)
{
	int i;

	for (i = 0; i < t->num_textures; i++) {
		free(t->textures[i]);
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
	struct textures *result = NULL;
	uint8_t *lump;
	size_t lump_len, min_len;
	uint32_t num_textures;
	int i;

	lump = vfreadall(input, &lump_len);
	vfclose(input);
	if (lump_len < 4) {
		ConversionError("Failed to read 4 byte lump header.");
		goto fail;
	}

	memcpy(&num_textures, lump, sizeof(uint32_t));
	SwapLE32(&num_textures);
	min_len = 4 + 4 * num_textures;
	if (lump_len < min_len) {
		ConversionError("Number of textures %d too large for lump "
		                "size:\n%d < %d", num_textures,
		                (int) lump_len, (int) min_len);
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
			ConversionError("Texture #%d overruns lump, start=%d"
			                "min len=%d, lump_len=%d", i, start,
			                (int) TextureLen(0), (int) lump_len);
			TX_FreeTextures(result);
			result = NULL;
			goto fail;
		}

		result->textures[i] =
			UnmarshalTexture(lump + start, lump_len - start);
		if (result->textures[i] == NULL) {
			ConversionError("Failed to unmarshal texture #%d", i);
			TX_FreeTextures(result);
			result = NULL;
			goto fail;
		}
	}

	TX_AddSerialNos(result);

fail:
	free(lump);
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
		SwapTexturePatches(swapped);
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

struct texture *TX_TextureForName(struct textures *txs, const char *name)
{
	int i;

	for (i = txs->num_textures - 1; i >= 0; --i) {
		if (!strncasecmp(txs->textures[i]->name, name, 8)) {
			return txs->textures[i];
		}
	}

	return NULL;
}

static void SetTextureName(struct texture *t, const char *name)
{
	char *namedest = t->name;
	int i;

	for (i = 0; i < 8; i++) {
		namedest[i] = toupper(name[i]);
		if (namedest[i] == '\0') {
			break;
		}
	}
}

bool TX_AddTexture(struct textures *txs, unsigned int pos, struct texture *t)
{
	if (TX_TextureForName(txs, t->name) != NULL) {
		return false;
	}

	txs->textures = checked_realloc(txs->textures,
		(txs->num_textures + 1) * sizeof(struct texture *));
	memmove(&txs->textures[pos + 1], &txs->textures[pos],
	        (txs->num_textures - pos) * sizeof(struct texture *));
	txs->textures[pos] = TX_DupTexture(t);

	txs->serial_nos = checked_realloc(txs->serial_nos,
		(txs->num_textures + 1) * sizeof(uint64_t));
	memmove(&txs->serial_nos[pos + 1], &txs->serial_nos[pos],
	        (txs->num_textures - pos) * sizeof(uint64_t));
	txs->serial_nos[pos] = NewSerialNo();

	SetTextureName(txs->textures[pos], t->name);

	++txs->num_textures;
	++txs->modified_count;

	return true;
}

void TX_RemoveTexture(struct textures *txs, unsigned int idx)
{
	if (idx >= txs->num_textures) {
		return;
	}

	free(txs->textures[idx]);
	memmove(&txs->textures[idx], &txs->textures[idx + 1],
	        (txs->num_textures - idx - 1) * sizeof(struct texture *));
	memmove(&txs->serial_nos[idx], &txs->serial_nos[idx + 1],
	        (txs->num_textures - idx - 1) * sizeof(uint64_t));
	--txs->num_textures;
	++txs->modified_count;
}

bool TX_RenameTexture(struct textures *txs, unsigned int idx,
                      const char *new_name)
{
	if (idx >= txs->num_textures) {
		return false;
	}

	if (TX_TextureForName(txs, new_name) != NULL) {
		return false;
	}

	SetTextureName(txs->textures[idx], new_name);

	++txs->modified_count;

	return true;
}
