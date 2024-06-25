//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// A texture bundle contains both a texture list and a pnames list.
// By always working in terms of bundles, we can reuse code between the
// texture and pnames directories.

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
#include "ui/dialog.h"

#include "textures/textures.h"
#include "textures/internal.h"

void TX_FreeBundle(struct texture_bundle *b)
{
	if (b->txs != NULL) {
		TX_FreeTextures(b->txs);
		b->txs = NULL;
	}
	if (b->pn != NULL) {
		TX_FreePnames(b->pn);
		b->pn = NULL;
	}
}

bool TX_BundleLoadPnames(struct texture_bundle *b, VFILE *in)
{
	b->txs = TX_NewTextureList(0);
	b->pn = TX_UnmarshalPnames(in);
	if (b->pn == NULL) {
		ConversionError("Failed to unmarshal PNAMES lump");
		return false;
	}
	return true;
}

bool TX_BundleLoadPnamesFrom(struct texture_bundle *b, struct directory *dir)
{
	struct wad_file *wf = VFS_WadFile(dir);
	VFILE *in;
	int lumpnum;

	// We explicitly use the WAD file interface to look up the PNAMES
	// lump. The reason is that if we are importing multiple lumps, we
	// may have imported PNAMES already but the version in the VFS
	// directory might not contain it yet.
	lumpnum = W_GetNumForName(wf, "PNAMES");
	if (lumpnum < 0) {
		ConversionError("To import a texture config, your WAD\n"
		                "must contain a PNAMES lump.");
		return false;
	}

	in = W_OpenLump(wf, lumpnum);
	if (in == NULL) {
		ConversionError("Failed to open PNAMES lump");
		return false;
	}

	return TX_BundleLoadPnames(b, in);
}

bool TX_BundleSavePnamesTo(struct texture_bundle *b, struct directory *dir)
{
	int lumpnum;
	struct wad_file *wf = VFS_WadFile(dir);
	VFILE *lump, *marshaled;

	// No need to save?
	if (b->pn->modified_count == 0) {
		return true;
	}

	lumpnum = W_GetNumForName(wf, "PNAMES");
	if (lumpnum < 0) {
		ConversionError("Can't find PNAMES lump to save changes.");
		return false;
	}

	lump = W_OpenLumpRewrite(wf, lumpnum);
	marshaled = TX_MarshalPnames(b->pn);
	vfcopy(marshaled, lump);
	vfclose(lump);
	vfclose(marshaled);
	return true;
}

bool TX_BundleLoadTextures(struct texture_bundle *b, struct directory *wad_dir,
                           VFILE *in)
{
	b->txs = NULL;
	b->pn = NULL;

	if (!TX_BundleLoadPnamesFrom(b, wad_dir)) {
		vfclose(in);
		return false;
	}

	b->txs = TX_UnmarshalTextures(in);
	if (b->txs == NULL) {
		ConversionError("Failed to unmarshal textures.");
		TX_FreeBundle(b);
		return false;
	}

	return true;
}

bool TX_BundleLoadTexturesFrom(struct texture_bundle *b,
                               struct directory *wad_dir,
                               struct directory_entry *ent)
{
	VFILE *in = VFS_OpenByEntry(wad_dir, ent);
	if (in == NULL) {
		ConversionError("Failed to open '%s' lump", ent->name);
		return NULL;
	}

	return TX_BundleLoadTextures(b, wad_dir, in);
}

bool TX_BundleParsePnames(struct texture_bundle *b, VFILE *in)
{
	b->txs = TX_NewTextureList(0);
	b->pn = TX_ParsePnamesConfig(in);
	if (b->pn == NULL) {
		ConversionError("Failed to parse PNAMES config");
		return false;
	}

	return true;
}

bool TX_BundleParseTextures(struct texture_bundle *b, VFILE *in)
{
	// We start with an empty pnames list and the texture parser adds
	// them as it finds them.
	b->pn = TX_ParsePnamesConfig(vfopenmem(NULL, 0));
	assert(b->pn != NULL);

	b->txs = TX_ParseTextureConfig(in, b->pn);
	if (b->txs == NULL) {
		ConversionError("Failed to parse texture config");
		TX_FreeBundle(b);
		return false;
	}

	return true;
}

bool TX_BundleConfirmAddPnames(struct texture_bundle *into,
                               struct texture_bundle *from)
{
	int missing = 0;
	int i;

	for (i = 0; i < from->pn->num_pnames; i++) {
		if (TX_GetPnameIndex(into->pn, from->pn->pnames[i]) < 0) {
			++missing;
		}
	}

	// We only prompt the user to confirm if there are PNAMES to add,
	// but not if it's PNAMEs we're merging anyway.
	return missing == 0
	    || (into->txs->num_textures == 0 && from->txs->num_textures == 0)
	    || UI_ConfirmDialogBox("Update PNAMES?", "Update", "Cancel",
	                           "%d patch names need to be added "
	                           "to PNAMES.\nProceed?", missing);
}

static bool TexturesIdentical(const struct texture *x, const struct texture *y)
{
	return x->patchcount == y->patchcount
	    && !memcmp(x, y, TX_TextureLen(x->patchcount));
}

bool TX_BundleConfirmTextureOverwrite(struct texture_bundle *into,
                                      struct texture_bundle *from)
{
	int differing = 0;
	int i, j;

	for (i = 0; i < from->txs->num_textures; i++) {
		struct texture *tx = TX_DupTexture(from->txs->textures[i]);
		int tnum;

		// Remap pname indexes. We have already made sure (above) that
		// any required patches have been added to PNAMES.
		for (j = 0; j < tx->patchcount; j++) {
			uint16_t *patchnum = &tx->patches[j].patch;
			char *pname = from->pn->pnames[*patchnum];
			*patchnum = TX_GetPnameIndex(into->pn, pname);
		}

		tnum = TX_TextureForName(into->txs, tx->name);
		if (tnum >= 0
		 && !TexturesIdentical(into->txs->textures[tnum], tx)) {
			++differing;
		}

		free(tx);
	}

	// We only prompt the user to confirm if there are PNAMES to add,
	// but not if it's PNAMEs we're merging anyway.
	return differing == 0
	    || UI_ConfirmDialogBox("Overwrite textures?", "Overwrite", "Cancel",
	                           "%d textures already exist.\n"
	                           "Overwrite them?", differing);
}

void TX_BundleMerge(struct texture_bundle *into, unsigned int position,
                    struct texture_bundle *from,
                    struct texture_bundle_merge_result *result)
{
	int i, j;

	memset(result, 0, sizeof(struct texture_bundle_merge_result));

	for (i = 0; i < from->pn->num_pnames; i++) {
		if (TX_GetPnameIndex(into->pn, from->pn->pnames[i]) < 0) {
			TX_AppendPname(into->pn, from->pn->pnames[i]);
			++result->pnames_added;
		} else {
			++result->pnames_present;
		}
	}

	for (i = 0; i < from->txs->num_textures; i++) {
		struct texture *tx = TX_DupTexture(from->txs->textures[i]);
		int existing_tnum;

		// Remap pname indexes. We have already made sure (above) that
		// any required patches have been added to PNAMES.
		for (j = 0; j < tx->patchcount; j++) {
			uint16_t *patchnum = &tx->patches[j].patch;
			char *pname = from->pn->pnames[*patchnum];
			*patchnum = TX_GetPnameIndex(into->pn, pname);
		}

		existing_tnum = TX_TextureForName(into->txs, tx->name);
		if (existing_tnum < 0) {
			TX_AddTexture(into->txs, position, tx);
			free(tx);
			++position;
			++result->textures_added;
		} else if (TexturesIdentical(
		               into->txs->textures[existing_tnum], tx)) {
			free(tx);
			++result->textures_present;
		} else {
			free(into->txs->textures[existing_tnum]);
			into->txs->textures[existing_tnum] = tx;
			++into->txs->modified_count;
			++result->textures_overwritten;
		}
	}
}
