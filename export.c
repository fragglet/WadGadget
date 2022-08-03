#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "dialog.h"
#include "dir_list.h"
#include "strings.h"

void PerformExport(struct blob_list *from, int from_index,
                   struct directory_listing *to)
{
	VFILE *fromlump, *tofile;
	FILE *f;
	char *filename, *extn;
	const struct blob_list_entry *dirent;

	// TODO
	if (BL_NumTagged(&from->tags) > 0) {
		UI_ConfirmDialogBox(
		    "Sorry", "Multi-export not implemented yet.");
		return;
	}

	dirent = from->get_entry(from, from_index);

	switch (dirent->type) {
	case BLOB_TYPE_FILE:
		extn = "";
		break;
	case BLOB_TYPE_LUMP:
		extn = ".lmp";
		// TODO: Convert to .gif/.wav etc.
		break;
	default:
		return;
	}
	// TODO: Export in other formats: .png, .wav, etc.
	filename = StringJoin("", DIR_GetPath(to), "/",
	                      dirent->name, extn, NULL);

	// TODO: Confirm file overwrite if already present.
	f = fopen(filename, "wb");
	if (f != NULL) {
		tofile = vfwrapfile(f);

		fromlump = from->open_blob(from, from_index);
		vfcopy(fromlump, tofile);
		vfclose(fromlump);
		vfclose(tofile);
	}

	free(filename);

	DIR_RefreshDirectory(to);
	// TODO: Mark new exported file(s) to highlight
}

