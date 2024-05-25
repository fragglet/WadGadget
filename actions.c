//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>

#include "actions.h"
#include "common.h"
#include "import.h"
#include "dialog.h"
#include "export.h"
#include "strings.h"
#include "vfs.h"

static void PerformCopy(struct directory_pane *active_pane,
                        struct directory_pane *other_pane, bool convert)
{
	struct directory *from = active_pane->dir, *to = other_pane->dir;
	struct file_set result = EMPTY_FILE_SET;

	// When we do an export or import, we create the new files/lumps
	// in the destination, and then switch to the other pane where they
	// are highlighted. The import/export functions both populate a
	// result set that contains the serial numbers of the new files.
	if (to->type == FILE_TYPE_DIR) {
		struct file_set *export_set =
			UI_DirectoryPaneTagged(active_pane);
		if (export_set->num_entries < 1) {
			UI_MessageBox(
			    "You have not selected anything to export.");
			VFS_FreeSet(&result);
			return;
		}
		if (PerformExport(from, export_set, to, &result, convert)) {
			UI_DirectoryPaneSetTagged(other_pane, &result);
			//SwitchToPane(!active_pane);
		}
		VFS_FreeSet(&result);
		return;
	}
	if (to->type == FILE_TYPE_WAD) {
		struct file_set *import_set =
			UI_DirectoryPaneTagged(active_pane);
		int to_point = UI_DirectoryPaneSelected(other_pane) + 1;
		if (import_set->num_entries < 1) {
			UI_MessageBox(
			    "You have not selected anything to import.");
			VFS_FreeSet(&result);
			return;
		}
		if (PerformImport(from, import_set, to, to_point,
		                  &result, convert)) {
			UI_DirectoryPaneSetTagged(other_pane, &result);
		//	SwitchToPane(!active_pane);
		}
		VFS_FreeSet(&result);
		return;
	}

	UI_MessageBox("Sorry, this isn't implemented yet.");
}

static void PerformCopyConvert(struct directory_pane *active_pane,
                               struct directory_pane *other_pane)
{
	PerformCopy(active_pane, other_pane, true);
}

static void PerformCopyNoConvert(struct directory_pane *active_pane,
                                 struct directory_pane *other_pane)
{
	PerformCopy(active_pane, other_pane, false);
}

const struct action copy_action = {
	KEY_F(5), 'O', "Copy",    "> Copy",
	PerformCopyConvert,
};
const struct action copy_noconv_action = {
	SHIFT_KEY_F(5), 0, "Copy",   "> Copy (no convert)",
	PerformCopyNoConvert,
};
const struct action export_action = {
	KEY_F(5), 'O', "Export",  "> Export to files",
	PerformCopyConvert,
};
const struct action export_noconv_action = {
	SHIFT_KEY_F(5), 0, "Export", "> Export (no convert)",
	PerformCopyNoConvert,
};
const struct action import_action = {
	KEY_F(5), 'O', "Import", "> Import",
	PerformCopyConvert,
};
const struct action import_noconv_action = {
	SHIFT_KEY_F(5), 0, "Import", "> Import (no convert)",
	PerformCopyNoConvert,
};

static char *CreateWadInDir(struct directory *from, struct file_set *from_set,
                            struct directory *to, bool convert)
{
	struct file_set result = EMPTY_FILE_SET;
	struct directory *newfile;
	char *filename, *filename2;

	filename = UI_TextInputDialogBox(
		"Make new WAD", 64,
		"Enter name for new WAD file:");

	if (filename == NULL) {
		return NULL;
	}

	// Forgot to include .wad extension?
	if (strchr(filename, '.') == NULL) {
		filename2 = StringJoin("", filename, ".wad", NULL);
		free(filename);
		filename = filename2;
	}

	if (VFS_EntryByName(to, filename) != NULL
	 && !UI_ConfirmDialogBox("Confirm Overwrite",
	                         "Overwrite existing '%s'?", filename)) {
		free(filename);
		return NULL;
	}

	filename2 = StringJoin("", to->path, "/", filename, NULL);

	if (!W_CreateFile(filename2)) {
		UI_MessageBox("%s\nFailed to create new WAD file.", filename);
		free(filename);
		free(filename2);
		return NULL;
	}
	newfile = VFS_OpenDir(filename2);
	if (newfile == NULL) {
		UI_MessageBox("%s\nFailed to open new file after creating.",
		              filename);
		free(filename);
		free(filename2);
		return NULL;
	}

	free(filename2);

	if (!PerformImport(from, from_set, newfile, 0, &result, convert)) {
		free(filename);
		filename = NULL;
	}
	VFS_Refresh(to);
	VFS_FreeSet(&result);
	VFS_CloseDir(newfile);

	return filename;
}

static void CreateWad(struct directory_pane *active_pane,
                      struct directory_pane *other_pane, bool convert)
{
	struct directory_pane *from_pane, *to_pane;
	struct file_set *import_set;
	char *filename;

	if (active_pane->dir->type == FILE_TYPE_WAD
	 && other_pane->dir->type == FILE_TYPE_DIR) {
		// Export from existing WAD to new WAD
		from_pane = active_pane;
		to_pane = other_pane;
		if (from_pane->tagged.num_entries == 0) {
			UI_MessageBox("You have not selected any "
			              "lumps to export.");
			return;
		}
	} else if (active_pane->dir->type == FILE_TYPE_DIR) {
		// Create new WAD and import tagged files.
		from_pane = active_pane;
		to_pane = active_pane;
		if (from_pane->tagged.num_entries == 0
		 && !UI_ConfirmDialogBox("Create WAD",
		                         "Create an empty WAD file?")) {
			return;
		}
	} else {
		return;
	}

	import_set = &from_pane->tagged;
	filename = CreateWadInDir(from_pane->dir, import_set, to_pane->dir,
	                          convert);
	if (filename != NULL) {
		UI_DirectoryPaneSearch(to_pane, filename);
		free(filename);
	//	SwitchToPane(panes[0] == to_pane ? 0 : 1);
	}
}

static void CreateWadConvert(struct directory_pane *active_pane,
                             struct directory_pane *other_pane)
{
	CreateWad(active_pane, other_pane, true);
}

static void CreateWadNoConvert(struct directory_pane *active_pane,
                               struct directory_pane *other_pane)
{
	CreateWad(active_pane, other_pane, false);
}

const struct action make_wad_action = {
	KEY_F(3), 'F', "MkWAD",  "Make WAD",
	CreateWadConvert,
};
const struct action make_wad_noconv_action = {
	SHIFT_KEY_F(3), 0, "MkWAD",  "Make WAD (no convert)",
	CreateWadNoConvert,
};
const struct action export_wad_action = {
	KEY_F(3), 'F', "ExpWAD",  "> Export as WAD",
	CreateWadConvert,
};
const struct action export_wad_noconv_action = {
	SHIFT_KEY_F(3), 0, "ExpWAD",  "> Export as WAD (no convert)",
	CreateWadNoConvert,
};

/* TODO -
const struct action view_action = {
	KEY_ENTER, 0,   NULL,       "View/Edit",
};

const struct action rearrange_action = {
	KEY_F(2), 'V', "Rearr",   "Move (rearrange)",
};
const struct action new_lump_action = {
	KEY_F(7), 'K', "NewLump", "New lump",
};

const struct action mkdir_action = {
	KEY_F(7), 'K', "Mkdir",  "Mkdir",
};
const struct action update_action = {
	KEY_F(4), 'U', "Upd",     "> Update",
};

const struct action rename_action = {
	KEY_F(6),  'B', "Ren",      "Rename",
};
const struct action delete_action = {
	KEY_F(8),  'X', "Del",      "Delete",
};
const struct action mark_action = {
	' ',       0,    NULL,      "Mark/unmark",
};
const struct action mark_pattern_action = {
	KEY_F(9),  'G', "MarkPat",  "Mark pattern",
};
const struct action unmark_all_action = {
	KEY_F(10), 'A', "UnmrkAll", "Unmark all",
};
*/
