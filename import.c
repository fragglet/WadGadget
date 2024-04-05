#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "dialog.h"
#include "import.h"
#include "strings.h"

void PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index)
{
	VFILE *fromfile, *tolump;
	char *p;
	struct directory_entry *dirent;
	struct wad_file *to_wad;
	char namebuf[9];

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	if (from_set->num_entries < 1) {
		UI_ConfirmDialogBox(
		    "Message", "You have not selected anything to import!");
		return;
	}
	if (from_set->num_entries > 1) {
		UI_ConfirmDialogBox(
		    "Sorry", "Multi-import not implemented yet.");
		return;
	}

	dirent = VFS_EntryBySerial(from, from_set->entries[0]);
	if (dirent == NULL) {
		return;
	}

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

