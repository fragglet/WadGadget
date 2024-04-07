#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>

#include "dialog.h"
#include "import.h"
#include "strings.h"

static void LumpNameForEntry(char *namebuf, struct directory_entry *ent)
{
	char *p;

	StringCopy(namebuf, ent->name, 9);

	switch (ent->type) {
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
		UI_ConfirmDialogBox("Error", "Can't import this file type");
		return;
	}
}

void PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index)
{
	VFILE *fromfile, *tolump;
	struct directory_entry *ent;
	struct wad_file *to_wad;
	char namebuf[9];
	int idx, lumpnum;

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	if (from_set->num_entries < 1) {
		UI_ConfirmDialogBox(
		    "Message", "You have not selected anything to import!");
		return;
	}

	to_wad = VFS_WadFile(to);
	lumpnum = to_index + 1;
	W_AddEntries(to_wad, lumpnum, from_set->num_entries);

	idx = 0;
	while ((ent = VFS_IterateSet(from, from_set, &idx)) != NULL) {
		LumpNameForEntry(namebuf, ent);
		W_SetLumpName(to_wad, lumpnum, namebuf);

		// TODO: This should be being done via VFS.
		fromfile = VFS_OpenByEntry(from, ent);
		tolump = W_OpenLumpRewrite(to_wad, lumpnum);
		vfcopy(fromfile, tolump);
		vfclose(fromfile);
		vfclose(tolump);
		++lumpnum;
	}

	VFS_Refresh(to);

	// TODO: Mark new imported lump(s) to highlight
}

