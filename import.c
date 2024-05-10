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
#include "strings.h"

static void LumpNameForEntry(char *namebuf, struct directory_entry *ent)
{
	char *p;

	StringCopy(namebuf, ent->name, 9);

	switch (ent->type) {
	case FILE_TYPE_LUMP:
		// WAD to WAD copy.
		break;
	case FILE_TYPE_FILE:
		// Lump name was set from filename, but we strip extension.
		p = strrchr(namebuf, '.');
		if (p != NULL) {
			*p = '\0';
		}
		// TODO: Convert from other formats: .png, .wav, etc.
		break;
	default:
		UI_MessageBox("Can't import this file type.");
		return;
	}
}

static const char *audio_extensions[] = {
	".aiff", ".wav", ".voc", ".flac", NULL,
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

static VFILE *PerformConversion(VFILE *input, struct directory_entry *ent)
{
	if (ent->type == FILE_TYPE_FILE) {
		if (HasExtension(ent->name, audio_extensions)) {
			return S_FromAudioFile(input);
		}
		if (StringHasSuffix(ent->name, ".png")) {
			return V_FromImageFile(input);
		}
	}

	return input;
}

bool PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index,
                   struct file_set *result, bool convert)
{
	VFILE *fromfile, *tolump;
	struct directory_entry *ent;
	struct wad_file *to_wad;
	struct wad_file_entry *waddir;
	char namebuf[9];
	int idx, lumpnum;

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	to_wad = VFS_WadFile(to);
	lumpnum = to_index;
	W_AddEntries(to_wad, lumpnum, from_set->num_entries);
	waddir = W_GetDirectory(to_wad);

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		LumpNameForEntry(namebuf, ent);
		W_SetLumpName(to_wad, lumpnum, namebuf);

		fromfile = VFS_OpenByEntry(from, ent);
		if (convert) {
			fromfile = PerformConversion(fromfile, ent);
		}
		// TODO: This should be being done via VFS.
		tolump = W_OpenLumpRewrite(to_wad, lumpnum);
		vfcopy(fromfile, tolump);
		vfclose(fromfile);
		vfclose(tolump);
		VFS_AddToSet(result, waddir[lumpnum].serial_no);
		++lumpnum;

		VFS_RemoveFromSet(from_set, ent->serial_no);
	}

	VFS_Refresh(to);
	return true;
}
