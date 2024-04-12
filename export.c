#include <curses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "audio.h"
#include "dialog.h"
#include "export.h"
#include "lump_info.h"
#include "strings.h"

static VFILE *PerformConversion(VFILE *input, const struct lump_type *lt)
{
	if (lt == &lump_type_sound) {
		return S_ToAudioFile(input);
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
                              struct directory_entry *ent)
{
	char *extn;

	if (ent->type == FILE_TYPE_LUMP) {
		if (lt == &lump_type_sound) {
			extn = ".wav";
		} else {
			extn = ".lmp";
		}
		// TODO: .png, etc.
	} else {
		extn = "";
	}

	return StringJoin("", ent->name, extn, NULL);
}

static bool ConfirmOverwrite(struct directory *from, struct file_set *from_set,
                             struct directory *to)
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
		filename = FileNameForEntry(lt, ent);
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

bool PerformExport(struct directory *from, struct file_set *from_set,
                   struct directory *to, struct file_set *result)
{
	VFILE *fromlump, *tofile;
	FILE *f;
	char *filename, *filename2;
	struct directory_entry *ent, *ent2;
	int idx;

	if (from->type == FILE_TYPE_DIR && !strcmp(from->path, to->path)) {
		UI_MessageBox(
		    "You can't copy to the same directory.");
		return false;
	}

	if (!ConfirmOverwrite(from, from_set, to)) {
		return false;
	}

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		const struct lump_type *lt = IdentifyLumpType(from, ent);
		filename = FileNameForEntry(lt, ent);
		if (filename == NULL) {
			continue;
		}
		filename2 = StringJoin("", to->path, "/", filename, NULL);
		free(filename);

		// TODO: This should be written through VFS.
		f = fopen(filename2, "wb");
		if (f != NULL) {
			tofile = vfwrapfile(f);

			fromlump = VFS_OpenByEntry(from, ent);
			fromlump = PerformConversion(fromlump, lt);
			vfcopy(fromlump, tofile);
			vfclose(fromlump);
			vfclose(tofile);
		}

		free(filename2);
	}

	VFS_Refresh(to);

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		const struct lump_type *lt = IdentifyLumpType(from, ent);
		VFS_RemoveFromSet(from_set, ent->serial_no);
		filename = FileNameForEntry(lt, ent);
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
