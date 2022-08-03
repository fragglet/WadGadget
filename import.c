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
	if (dirent->type != BLOB_TYPE_LUMP
	 && dirent->type != BLOB_TYPE_FILE) {
		return;
	}

	W_AddEntries(to, to_index, 1);

	fromfile = from->open_blob(from, from_index);
	// TODO: Import and convert from other formats: .png, .wav, etc.

	tolump = W_OpenLumpRewrite(to, to_index);
	vfcopy(fromfile, tolump);
	vfclose(fromfile);
	vfclose(tolump);

	// Lump name gets set from filename, but we strip extension.
	StringCopy(namebuf, dirent->name, sizeof(namebuf));
	p = strrchr(namebuf, '.');
	if (p != NULL) {
		*p = '\0';
	}

	W_SetLumpName(to, to_index, namebuf);

	// TODO: Mark new imported lump(s) to highlight
}

