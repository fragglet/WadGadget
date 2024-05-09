//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "dialog.h"
#include "export.h"
#include "graphic.h"
#include "lump_info.h"
#include "mus2mid.h"
#include "strings.h"

static VFILE *PerformConversion(VFILE *input, const struct lump_type *lt)
{
	if (lt == &lump_type_sound) {
		return S_ToAudioFile(input);
	} else if (lt == &lump_type_graphic) {
		return V_ToImageFile(input);
	} else if (lt == &lump_type_mus) {
		VFILE *result = vfopenmem(NULL, 0);
		if (mus2mid(input, result)) {
			vfclose(input);
			vfclose(result);
			return NULL;
		}
		vfseek(result, 0, SEEK_SET);
		vfclose(input);
		return result;
	} else {
		return input;
	}
}

static const struct lump_type *IdentifyLumpType(struct directory *dir,
                                                struct directory_entry *ent)
{
	struct wad_file *wf;
	unsigned int idx;

	if (ent->type != FILE_TYPE_LUMP) {
		return &lump_type_unknown;
	}

	wf = VFS_WadFile(dir);
	idx = ent - dir->entries;

	return LI_IdentifyLump(wf, idx);
}

static char *FileNameForEntry(const struct lump_type *lt,
                              struct directory_entry *ent,
                              bool convert)
{
	const char *extn;

	if (ent->type != FILE_TYPE_LUMP) {
		extn = "";
	} else {
		extn = LI_GetExtension(lt, convert);
	}

	return StringJoin("", ent->name, extn, NULL);
}

static bool ConfirmOverwrite(struct directory *from, struct file_set *from_set,
                             struct directory *to, bool convert)
{
	struct file_set overwrite_set = EMPTY_FILE_SET;
	const struct lump_type *lt;
	struct directory_entry *ent;
	bool result;
	char buf[64];
	char *filename;
	int i;

	VFS_Refresh(to);

	for (i = 0; i < from_set->num_entries; i++) {
		ent = VFS_EntryBySerial(from, from_set->entries[i]);
		if (ent == NULL) {
			continue;
		}
		if (ent->type == FILE_TYPE_DIR) {
			UI_MessageBox("You can't copy directories.");
			return false;
		}
		lt = IdentifyLumpType(from, ent);
		filename = FileNameForEntry(lt, ent, convert);
		if (filename == NULL) {
			continue;
		}
		ent = VFS_EntryByName(to, filename);
		if (ent != NULL) {
			VFS_AddToSet(&overwrite_set, ent->serial_no);
		}

		free(filename);
	}

	if (overwrite_set.num_entries == 0) {
		// Nothing to overwrite.
		return true;
	}

	VFS_DescribeSet(to, &overwrite_set, buf, sizeof(buf));
	result = UI_ConfirmDialogBox("Confirm Overwrite", "Overwrite %s?", buf);
	VFS_FreeSet(&overwrite_set);
	return result;
}

bool ExportToFile(struct directory *from, struct directory_entry *ent,
                  const struct lump_type *lt, const char *to_filename,
                  bool convert)
{
	VFILE *fromlump, *tofile;
	FILE *f;

	fromlump = VFS_OpenByEntry(from, ent);
	if (convert) {
		fromlump = PerformConversion(fromlump, lt);
	}
	if (fromlump == NULL) {
		// TODO: Print an error message on failed conversion
		return false;
	}

	// TODO: This should be written through VFS.
	f = fopen(to_filename, "wb");
	if (f == NULL) {
		vfclose(fromlump);
		return false;
	}

	tofile = vfwrapfile(f);
	vfcopy(fromlump, tofile);
	vfclose(fromlump);
	vfclose(tofile);

	return true;
}

bool PerformExport(struct directory *from, struct file_set *from_set,
                   struct directory *to, struct file_set *result,
                   bool convert)
{
	char *filename, *filename2;
	struct directory_entry *ent, *ent2;
	bool success;
	int idx;

	if (from->type == FILE_TYPE_DIR && !strcmp(from->path, to->path)) {
		UI_MessageBox(
		    "You can't copy to the same directory.");
		return false;
	}

	if (!ConfirmOverwrite(from, from_set, to, convert)) {
		return false;
	}

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		const struct lump_type *lt = IdentifyLumpType(from, ent);
		filename = FileNameForEntry(lt, ent, convert);
		if (filename == NULL) {
			continue;
		}
		filename2 = StringJoin("", to->path, "/", filename, NULL);
		free(filename);
		success = ExportToFile(from, ent, lt, filename2, convert);
		free(filename2);
		if (!success) {
			return false;
		}
	}

	VFS_Refresh(to);

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		const struct lump_type *lt = IdentifyLumpType(from, ent);
		VFS_RemoveFromSet(from_set, ent->serial_no);
		filename = FileNameForEntry(lt, ent, convert);
		if (filename == NULL) {
			continue;
		}
		ent2 = VFS_EntryByName(to, filename);
		free(filename);
		if (ent2 != NULL) {
			VFS_AddToSet(result, ent2->serial_no);
		}
	}

	return true;
}
