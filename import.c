#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dialog.h"
#include "import.h"
#include "strings.h"

void PerformImport(struct blob_list *from, int from_index,
                   struct wad_file *to, int to_index)
{
	VFILE *fromfile, *tolump;
	char *p;
	const struct blob_list_entry *dirent;
	char namebuf[9];

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	// TODO
	if (BL_NumTagged(&from->tags) > 0) {
		UI_ConfirmDialogBox(
		    "Sorry", "Multi-import not implemented yet.");
		return;
	}

	if (from_index < 0 || to_index < 0) {
		return;
	}

	dirent = from->get_entry(from, from_index);
	StringCopy(namebuf, dirent->name, sizeof(namebuf));

	switch (dirent->type) {
	case BLOB_TYPE_LUMP:
		// WAD to WAD copy.
		break;
	case BLOB_TYPE_FILE:
		// Lump name was set from filename, but we strip extension.
		p = strrchr(namebuf, '.');
		if (p != NULL) {
			*p = '\0';
		}
		// TODO: Convert from other formats: .png, .wav, etc.
	default:
		return;
	}

	W_AddEntries(to, to_index, 1);

	fromfile = from->open_blob(from, from_index);

	tolump = W_OpenLumpRewrite(to, to_index);
	vfcopy(fromfile, tolump);
	vfclose(fromfile);
	vfclose(tolump);
	W_SetLumpName(to, to_index, namebuf);

	// TODO: Mark new imported lump(s) to highlight
}

