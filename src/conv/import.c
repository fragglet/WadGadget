//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "conv/import.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "conv/audio.h"
#include "conv/error.h"
#include "conv/graphic.h"
#include "conv/palette.h"
#include "fs/vfs.h"
#include "fs/wad_file.h"
#include "palette/palette.h"
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
		StringUpper(namebuf);
		break;
	default:
		ConversionError("File type %d cannot be imported", ent->type);
		return;
	}
}

static const char *audio_extensions[] = {
    // All the formats supported by libsndfile, except lossily-
    // compressed formats, because you shouldn't be using those.
    ".aiff", ".wav", ".voc", ".flac", ".aiff", ".svx", ".au",  ".paf",
    ".sf",   ".w64", ".pvf", ".xi",   ".caf",  ".wve", ".dwd", ".txw",
    ".sds",  ".avr", ".htk", ".rex",  ".rx2",  ".snd", NULL,
};

static const char *lump_extensions[] = {
    ".lmp",
    ".mus",
    NULL,
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
	const struct palette *pal = PAL_PaletteForWAD(to_wad);
	src_name = PathBaseName(src_name);

	if (HasExtension(src_name, lump_extensions)) {
		return input;
	} else if (HasExtension(src_name, audio_extensions)) {
		return S_FromAudioFile(input);
	} else if (!strcasecmp(src_name, "playpal.png")) {
		return V_PaletteFromImageFile(input);
	} else if (!strcasecmp(src_name, "colormap.png") ||
	           StringHasSuffix(src_name, ".cmap.png")) {
		return V_ColormapFromImageFile(input, pal);
	} else if (StringHasSuffix(src_name, ".flat.png")) {
		return V_FlatFromImageFile(input, pal);
	} else if (StringHasSuffix(src_name, ".fullscreen.png")) {
		return V_FullscreenFromImageFile(input, pal);
	} else if (StringHasSuffix(src_name, ".png")) {
		return V_FromImageFile(input, pal);
	} else if (!strcasecmp(src_name, "PNAMES.txt")) {
		return ConvertPnames(input);
	} else if (!strncasecmp(src_name, "TEXTURE", 7) &&
	           StringHasSuffix(src_name, ".txt")) {
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

struct update_mapping {
	struct directory_entry *from_ent;
	int to_lumpnum;
};

// AddLumpsMapping inserts new lumps for all entries in `from_set` and returns
// an update mapping that will update those new lumps.
static struct update_mapping *AddLumpsMapping(
	struct directory *from, struct file_set *from_set,
	struct directory *to, int insert_index)
{
	struct update_mapping *result;
	struct directory_entry *ent;
	struct wad_file *wf;
	int lumpnum, idx, m;
	char namebuf[9];

	result = calloc(from_set->num_entries + 1,
	                sizeof(struct update_mapping));

	lumpnum = insert_index;
	wf = VFS_WadFile(to);
	W_AddEntries(wf, insert_index, from_set->num_entries);

	idx = 0;
	m = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		LumpNameForEntry(namebuf, ent);
		result[m].from_ent = ent;
		result[m].to_lumpnum = lumpnum;
		W_SetLumpName(wf, lumpnum, namebuf);
		++m;
		++lumpnum;
	}

	VFS_Refresh(to);

	return result;
}

static bool ApplyUpdateMapping(struct progress_window *progress,
                               struct directory *from,
                               struct file_set *from_set,
                               struct directory *to,
                               struct update_mapping *um,
                               struct file_set *result, bool convert)
{
	struct wad_file_entry *waddir;
	VFILE *from_file;
	struct directory_entry *ent;
	struct wad_file *to_wad = VFS_WadFile(to);
	int i, lumpnum;

	// We only ever do conversions when importing from files.
	convert = convert && from->type == FILE_TYPE_DIR;
	waddir = W_GetDirectory(to_wad);

	for (i = 0; um[i].from_ent != NULL; ++i) {
		ent = um[i].from_ent;
		lumpnum = um[i].to_lumpnum;
		from_file = VFS_OpenByEntry(from, ent);

		if (!ImportFromFile(from_file, ent->name, to, lumpnum,
		                    convert)) {
			VFS_Rollback(to);
			return false;
		}

		VFS_AddToSet(result, waddir[lumpnum].serial_no);
		++lumpnum;

		VFS_RemoveFromSet(from_set, ent->serial_no);
		UI_UpdateProgressWindow(progress, ent->name);
	}

	VFS_Refresh(to);
	return true;
}

bool PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index, struct file_set *result,
                   bool convert)
{
	VFILE *from_file;
	struct directory_entry *ent;
	struct wad_file *to_wad = VFS_WadFile(to);
	struct wad_file_entry *waddir;
	struct progress_window progress;
	char namebuf[9];
	int idx, lumpnum;

	UI_InitProgressWindow(&progress, from_set->num_entries,
	                      from->type == FILE_TYPE_DIR ? "Importing"
	                                                  : "Copying");

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

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

// BuildUpdateMapping is used by PerformUpdateWAD below to generate a mapping
// list, from the source file (from_ent) to the index lump# in the destination
// WAD. Any that can't be matched are stored in missing_lumps.
static struct update_mapping *BuildUpdateMapping(
	struct directory *from, struct file_set *from_set,
	struct directory *to, struct file_set *missing_lumps)
{
	struct directory_entry *ent;
	struct update_mapping *result;
	char namebuf[9];
	int idx, m;

	result = calloc(from_set->num_entries + 1,
	                sizeof(struct update_mapping));

	idx = 0;
	m = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		struct directory_entry *to_ent;

		LumpNameForEntry(namebuf, ent);
		// TODO: Check for duplicate lumps with the same name
		// TODO: Correctly handle lumps belonging to levels. For
		// example, if I select MAP01/LINEDEFS and hit update, it
		// should *only* update to MAP01/LINEDEFS on the other side,
		// not any other random LINEDEFS lump.
		to_ent = VFS_EntryByName(to, namebuf);

		if (to_ent != NULL) {
			result[m].from_ent = ent;
			result[m].to_lumpnum = to_ent - to->entries;
			++m;
		} else {
			VFS_AddToSet(missing_lumps, ent->serial_no);
		}
	}

	return result;
}

bool PerformUpdateWAD(struct directory *from, struct file_set *from_set,
                      struct directory *to, int to_index,
                      struct file_set *result, bool convert)
{
	struct update_mapping *um;
	bool success;
	struct progress_window progress;
	struct file_set missing_lumps = EMPTY_FILE_SET;
	char buf[64];

	UI_InitProgressWindow(&progress, from_set->num_entries, "Updating");

	um = BuildUpdateMapping(from, from_set, to, &missing_lumps);
	if (um == NULL) {
		return false;
	}

	VFS_DescribeSet(from, &missing_lumps, buf, sizeof(buf));

	if (missing_lumps.num_entries > 0
	 && !UI_ConfirmDialogBox("Confirm Add Lumps", "Add Lumps", "Cancel",
	                         "%s not found in destination WAD.\n"
	                         "Add missing lump(s)?", buf)) {
		VFS_FreeSet(&missing_lumps);
		free(um);
		return false;
	}

	success = ApplyUpdateMapping(&progress, from, from_set, to,
	                             um, result, convert);
	free(um);

	if (success && missing_lumps.num_entries > 0) {
		um = AddLumpsMapping(from, &missing_lumps, to, to_index);
		success = ApplyUpdateMapping(&progress, from, from_set, to,
		                             um, result, convert);
		free(um);
	}

	VFS_FreeSet(&missing_lumps);

	return success;
}
