#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dialog.h"
#include "import.h"
#include "strings.h"

void PerformImport(struct directory *from, int from_index,
                   struct directory *to, int to_index)
{
	VFILE *fromfile, *tolump;
	char *p;
	struct directory_entry *dirent;
	struct wad_file *to_wad;
	char namebuf[9];

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	/*
	// TODO
	if (BL_NumTagged(&from->tags) > 0) {
		UI_ConfirmDialogBox(
		    "Sorry", "Multi-import not implemented yet.");
		return;
	}
	*/

	if (from_index < 0 || to_index < 0) {
		return;
	}

	dirent = &from->entries[from_index];
	StringCopy(namebuf, dirent->name, sizeof(namebuf));

	switch (dirent->type) {
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
		return;
	}

	// TODO: This should be being done via VFS.
	to_wad = VFS_WadFile(to);
	W_AddEntries(to_wad, to_index, 1);

	fromfile = VFS_OpenByEntry(from, dirent);
	tolump = W_OpenLumpRewrite(to_wad, to_index);
	vfcopy(fromfile, tolump);
	vfclose(fromfile);
	vfclose(tolump);
	W_SetLumpName(to_wad, to_index, namebuf);
	VFS_Refresh(to);

	// TODO: Mark new imported lump(s) to highlight
}

