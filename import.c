#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
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
		UI_MessageBox("Can't import this file type.");
		return;
	}
}

bool PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index,
                   struct file_set *result)
{
	VFILE *fromfile, *tolump;
	struct directory_entry *ent;
	struct wad_file *to_wad;
	struct wad_file_entry *waddir;
	char namebuf[9];
	int idx, lumpnum;

	// TODO: Update/overwrite existing lump instead of creating a new
	// lump.

	if (from_set->num_entries < 1) {
		UI_MessageBox(
		    "You have not selected anything to import.");
		return false;
	}

	to_wad = VFS_WadFile(to);
	lumpnum = to_index + 1;
	W_AddEntries(to_wad, lumpnum, from_set->num_entries);
	waddir = W_GetDirectory(to_wad);

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
		VFS_AddToSet(result, waddir[lumpnum].serial_no);
		++lumpnum;

		VFS_RemoveFromSet(from_set, ent->serial_no);
	}

	VFS_Refresh(to);
	return true;
}
