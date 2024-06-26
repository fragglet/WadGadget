//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

#include "common.h"
#include "conv/audio.h"
#include "conv/error.h"
#include "conv/graphic.h"
#include "conv/import.h"
#include "conv/palette.h"
#include "lump_info.h"
#include "stringlib.h"
#include "textures/textures.h"
#include "ui/dialog.h"

static void LumpNameForEntry(char *namebuf, struct directory_entry *ent)
{
	char *p;

	StringCopy(namebuf, ent->name, 9);

	switch (ent->type) {
	case FILE_TYPE_LUMP:
		// WAD to WAD copy.
		break;
	case FILE_TYPE_WAD:
		// It's weird to import a WAD into a WAD, but there's no
		// reason to forbid it.
		/* fallthrough */
	case FILE_TYPE_FILE:
		// Lump name was set from filename, but we strip extension.
		p = strrchr(namebuf, '.');
		if (p != NULL) {
			*p = '\0';
		}
		break;
	default:
		ConversionError("File type %d cannot be imported", ent->type);
		return;
	}
}

static const char *audio_extensions[] = {
	// All the formats supported by libsndfile, except lossily-
	// compressed formats, because you shouldn't be using those.
	".aiff", ".wav", ".voc", ".flac", ".aiff", ".svx", ".au", ".paf",
	".sf", ".w64", ".pvf", ".xi", ".caf", ".wve", ".dwd", ".txw",
	".sds", ".avr", ".htk", ".rex", ".rx2", ".snd", NULL,
};

static const char *lump_extensions[] = {
	".lmp", ".mus", NULL,
};

static bool HasExtension(const char *filename, const char **exts)
{
	int i;

	for (i = 0; exts[i] != NULL; i++) {
		if (StringHasSuffix(filename, exts[i])) {
			return true;
		}
	}

	return false;
}

static VFILE *ConvertPnames(VFILE *input)
{
	VFILE *result;
	struct pnames *pn = TX_ParsePnamesConfig(input);

	if (pn == NULL) {
		ConversionError("Failed to parse PNAMES config");
		return NULL;
	}

	result = TX_MarshalPnames(pn);
	TX_FreePnames(pn);
	return result;
}

static VFILE *ImportTextures(VFILE *input, struct directory *to_wad)
{
	struct texture_bundle into, from;
	struct texture_bundle_merge_result merge_stats;
	VFILE *result = NULL;

	if (!TX_BundleLoadPnamesFrom(&into, to_wad)) {
		vfclose(input);
		return NULL;
	}

	if (!TX_BundleParseTextures(&from, input)) {
		goto fail;
	}

	if (!TX_BundleConfirmAddPnames(&into, &from)) {
		goto fail;
	}

	TX_BundleMerge(&into, 0, &from, &merge_stats);

	if (!TX_BundleSavePnamesTo(&into, to_wad)) {
		goto fail;
	}

	result = TX_MarshalTextures(into.txs);
fail:
	TX_FreeBundle(&into);
	TX_FreeBundle(&from);
	return result;
}

static VFILE *PerformConversion(VFILE *input, struct directory *to_wad,
                                const char *src_name)
{
	src_name = PathBaseName(src_name);

	if (HasExtension(src_name, lump_extensions)) {
		return input;
	} else if (HasExtension(src_name, audio_extensions)) {
		return S_FromAudioFile(input);
	} else if (!strcasecmp(src_name, "playpal.png")) {
		return V_PaletteFromImageFile(input);
	} else if (StringHasSuffix(src_name, ".flat.png")) {
		return V_FlatFromImageFile(input);
	} else if (StringHasSuffix(src_name, ".fullscreen.png")) {
		return V_FullscreenFromImageFile(input);
	} else if (StringHasSuffix(src_name, ".png")) {
		return V_FromImageFile(input);
	} else if (!strcasecmp(src_name, "PNAMES.txt")) {
		return ConvertPnames(input);
	} else if (!strncasecmp(src_name, "TEXTURE", 7)
	        && StringHasSuffix(src_name, ".txt")) {
		return ImportTextures(input, to_wad);
	}

	return input;
}

bool ImportFromFile(VFILE *from_file, const char *src_name,
                    struct directory *to_wad, int lumpnum, bool convert)
{
	VFILE *to_lump;

	if (convert) {
		from_file = PerformConversion(from_file, to_wad, src_name);
	}
	if (from_file == NULL) {
		ConversionError("Failed conversion for '%s'", src_name);
		return false;
	}

	to_lump = W_OpenLumpRewrite(VFS_WadFile(to_wad), lumpnum);
	vfcopy(from_file, to_lump);
	vfclose(from_file);
	vfclose(to_lump);
	return true;
}

bool PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index,
                   struct file_set *result, bool convert)
{
	VFILE *from_file;
	struct directory_entry *ent;
	struct wad_file *to_wad;
	struct wad_file_entry *waddir;
	struct progress_window progress;
	char namebuf[9];
	int idx, lumpnum;

	UI_InitProgressWindow(
		&progress, from_set->num_entries,
		from->type == FILE_TYPE_DIR ? "Importing" : "Copying");

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	to_wad = VFS_WadFile(to);
	lumpnum = to_index;
	W_AddEntries(to_wad, lumpnum, from_set->num_entries);
	VFS_Refresh(to);
	waddir = W_GetDirectory(to_wad);

	// We only ever do conversions when importing from files.
	convert = convert && from->type == FILE_TYPE_DIR;

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {

		LumpNameForEntry(namebuf, ent);
		W_SetLumpName(to_wad, lumpnum, namebuf);

		from_file = VFS_OpenByEntry(from, ent);

		if (!ImportFromFile(from_file, ent->name, to, lumpnum,
		                    convert)) {
			VFS_Rollback(to);
			return false;
		}

		VFS_AddToSet(result, waddir[lumpnum].serial_no);
		++lumpnum;

		VFS_RemoveFromSet(from_set, ent->serial_no);
		UI_UpdateProgressWindow(&progress, ent->name);
	}

	VFS_Refresh(to);
	return true;
}
