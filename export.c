#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "export.h"
#include "strings.h"

void PerformExport(struct directory *from, int from_index,
                   struct directory *to)
{
	VFILE *fromlump, *tofile;
	FILE *f;
	char *filename, *extn;
	struct directory_entry *dirent;

	/*
	// TODO
	if (BL_NumTagged(&from->tags) > 0) {
		UI_ConfirmDialogBox(
		    "Sorry", "Multi-export not implemented yet.");
		return;
	}
	*/

	dirent = &from->entries[from_index];

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

