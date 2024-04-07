#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "export.h"
#include "strings.h"

static char *FileNameForEntry(struct directory_entry *dirent)
{
	char *extn;

	switch (dirent->type) {
	case FILE_TYPE_FILE:
	case FILE_TYPE_WAD:
		extn = "";
		break;
	case FILE_TYPE_LUMP:
		extn = ".lmp";
		// TODO: Convert to .png/.wav etc.
		break;
	default:
		return NULL;
	}

	return StringJoin("", dirent->name, extn, NULL);
}

static bool ConfirmOverwrite(struct directory *from, struct file_set *from_set,
                             struct directory *to)
{
	struct file_set overwrite_set = EMPTY_FILE_SET;
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
		filename = FileNameForEntry(ent);
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
	result = UI_ConfirmDialogBox("Message", "Overwrite %s?", buf);
	VFS_FreeSet(&overwrite_set);
	return result;
}

void PerformExport(struct directory *from, struct file_set *from_set,
                   struct directory *to, struct file_set *result)
{
	VFILE *fromlump, *tofile;
	FILE *f;
	char *filename, *filename2;
	struct directory_entry *ent, *ent2;
	int idx;

	if (from_set->num_entries < 1) {
		UI_ConfirmDialogBox(
		    "Message", "You have not selected anything to export!");
		return;
	}
	if (from->type == FILE_TYPE_DIR && !strcmp(from->path, to->path)) {
		UI_ConfirmDialogBox(
		    "Message", "You can't copy to the same directory!");
		return;
	}

	if (!ConfirmOverwrite(from, from_set, to)) {
		return;
	}

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		filename = FileNameForEntry(ent);
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
			vfcopy(fromlump, tofile);
			vfclose(fromlump);
			vfclose(tofile);
		}

		free(filename2);
	}

	VFS_Refresh(to);

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		VFS_RemoveFromSet(from_set, ent->serial_no);
		filename = FileNameForEntry(ent);
		if (filename == NULL) {
			continue;
		}
		ent2 = VFS_EntryByName(to, filename);
		free(filename);
		if (ent2 != NULL) {
			VFS_AddToSet(result, ent2->serial_no);
		}
	}
}
