#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "export.h"
#include "strings.h"

void PerformExport(struct directory *from, struct file_set *from_set,
                   struct directory *to)
{
	VFILE *fromlump, *tofile;
	FILE *f;
	char *filename, *extn;
	struct directory_entry *dirent;

	if (from_set->num_entries < 1) {
		UI_ConfirmDialogBox(
		    "Message", "You have not selected anything to export!");
		return;
	}
	if (from_set->num_entries > 1) {
		UI_ConfirmDialogBox(
		    "Sorry", "Multi-export not implemented yet.");
		return;
	}

	dirent = VFS_EntryBySerial(from, from_set->entries[0]);
	if (dirent == NULL) {
		return;
	}

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
		return;
	}
	filename = StringJoin("", to->path, "/",
	                      dirent->name, extn, NULL);

	// TODO: Confirm file overwrite if already present.
	// TODO: This should be written through VFS.
	f = fopen(filename, "wb");
	if (f != NULL) {
		tofile = vfwrapfile(f);

		fromlump = VFS_OpenByEntry(from, dirent);
		vfcopy(fromlump, tofile);
		vfclose(fromlump);
		vfclose(tofile);
	}

	free(filename);

	VFS_Refresh(to);
	// TODO: Mark new exported file(s) to highlight
}

