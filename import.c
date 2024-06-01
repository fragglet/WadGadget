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

#include "audio.h"
#include "dialog.h"
#include "import.h"
#include "graphic.h"
#include "lump_info.h"
#include "strings.h"
#include "textures.h"

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
		UI_MessageBox("'%s' has a file type that\ncan't be imported.",
		              ent->name);
		return;
	}
}

static const char *audio_extensions[] = {
	".aiff", ".wav", ".voc", ".flac", NULL,
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

static VFILE *PerformConversion(VFILE *input, const char *src_name,
                                bool flats_section)
{
	if (HasExtension(src_name, lump_extensions)) {
		return input;
	} else if (HasExtension(src_name, audio_extensions)) {
		return S_FromAudioFile(input);
	} else if (StringHasSuffix(src_name, ".png")) {
		if (flats_section) {
			return V_FlatFromImageFile(input);
		} else {
			return V_FromImageFile(input);
		}
	} else if (!strncasecmp(src_name, "TEXTURE", 7)) {
		return TX_FromTexturesConfig(input);
	}

	return input;
}

bool ImportFromFile(VFILE *from_file, const char *src_name,
                    struct wad_file *to_wad, int lumpnum,
                    bool flats_section, bool convert)
{
	VFILE *to_lump;

	if (convert) {
		from_file = PerformConversion(from_file, src_name,
		                              flats_section);
	}
	if (from_file == NULL) {
		return false;
	}

	to_lump = W_OpenLumpRewrite(to_wad, lumpnum);
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
	bool flats_section;

	UI_InitProgressWindow(
		&progress, from_set->num_entries,
		from->type == FILE_TYPE_DIR ? "Importing" : "Copying");

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	to_wad = VFS_WadFile(to);
	lumpnum = to_index;
	W_AddEntries(to_wad, lumpnum, from_set->num_entries);
	waddir = W_GetDirectory(to_wad);

	// We only ever do conversions when importing from files.
	convert = convert && from->type == FILE_TYPE_DIR;
	flats_section = LI_LumpInSection(to_wad, to_index,
	                                 &lump_section_flats);

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {

		LumpNameForEntry(namebuf, ent);
		W_SetLumpName(to_wad, lumpnum, namebuf);

		from_file = VFS_OpenByEntry(from, ent);

		if (!ImportFromFile(from_file, ent->name, to_wad, lumpnum,
		                    flats_section, convert)) {
			// TODO: Delete the new entries we added?
			// TODO: Show an error message
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
