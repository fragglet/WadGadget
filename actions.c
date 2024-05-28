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
#include <sys/stat.h>

#include "actions.h"
#include "common.h"
#include "import.h"
#include "dialog.h"
#include "export.h"
#include "pane.h"
#include "strings.h"
#include "vfs.h"
#include "view.h"

#define WAD_JUNK_THRESHOLD_KB  500

extern void SwitchToPane(struct directory_pane *pane); // in wadgadget.c
extern void ReplacePane(struct directory_pane *old_pane,
                        struct directory_pane *new_pane);

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
			SwitchToPane(other_pane);
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
			SwitchToPane(other_pane);
			VFS_CommitChanges(to);
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
	KEY_F(5), 'O', "Copy", "> Copy",
	PerformCopyConvert,
};
const struct action copy_noconv_action = {
	SHIFT_KEY_F(5), 0, NULL, "> Copy (no convert)",
	PerformCopyNoConvert,
};
const struct action export_action = {
	KEY_F(5), 'O', "Export", "> Export",
	PerformCopyConvert,
};
const struct action export_noconv_action = {
	SHIFT_KEY_F(5), 0, NULL, "> Export (no convert)",
	PerformCopyNoConvert,
};
const struct action import_action = {
	KEY_F(5), 'O', "Import", "> Import",
	PerformCopyConvert,
};
const struct action import_noconv_action = {
	SHIFT_KEY_F(5), 0, NULL, "> Import (no convert)",
	PerformCopyNoConvert,
};

static void PerformUpdate(struct directory_pane *active_pane,
                          struct directory_pane *other_pane)
{
	UI_MessageBox("Sorry, not implemented yet.");
}

const struct action update_action = {
	KEY_F(3), 'U', "Upd", "> Update",
	PerformUpdate,
};

static void PerformMkdir(struct directory_pane *active_pane,
                         struct directory_pane *other_pane)
{
	char *input_filename, *filename;

	input_filename = UI_TextInputDialogBox(
	    "Make directory", "Create", 30, "Name for new directory?");
	if (input_filename == NULL) {
		return;
	}
	filename = StringJoin("/", active_pane->dir->path, input_filename,
	                      NULL);
	mkdir(filename, 0777);
	VFS_Refresh(active_pane->dir);
	UI_DirectoryPaneSelectByName(active_pane, input_filename);
	free(input_filename);
	free(filename);
}

const struct action mkdir_action = {
	KEY_F(7), 'K', "Mkdir", ". Make directory",
	PerformMkdir,
};

static char *CreateWadInDir(struct directory *from, struct file_set *from_set,
                            struct directory *to, bool convert)
{
	struct file_set result = EMPTY_FILE_SET;
	struct directory *newfile;
	char *filename, *filename2;

	filename = UI_TextInputDialogBox(
		"Make new WAD", "Create", 30,
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
	 && !UI_ConfirmDialogBox("Confirm Overwrite", "Overwrite", "Cancel",
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

	if (PerformImport(from, from_set, newfile, 0, &result, convert)) {
		VFS_CommitChanges(newfile);
	} else {
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
		 && !UI_ConfirmDialogBox("Create WAD", "Create", "Cancel",
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
		SwitchToPane(to_pane);
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
	KEY_F(9), 'F', "MkWAD", ". Make WAD",
	CreateWadConvert,
};
const struct action make_wad_noconv_action = {
	SHIFT_KEY_F(9), 0, NULL, ". Make WAD (no convert)",
	CreateWadNoConvert,
};
const struct action export_wad_action = {
	KEY_F(9), 'F', "ExpWAD", ".> Export as WAD",
	CreateWadConvert,
};

static void MoveLumps(struct directory_pane *p, struct wad_file *wf)
{
	struct wad_file_entry *dir;
	int i, j, numlumps;
	int insert_start, insert_end, insert_point;

	insert_start = UI_DirectoryPaneSelected(p) + 1;
	insert_end = insert_start + p->tagged.num_entries;
	insert_point = insert_start;

	// TODO: Should this be done through VFS?
	W_AddEntries(wf, insert_start, p->tagged.num_entries);
	dir = W_GetDirectory(wf);
	numlumps = W_NumLumps(wf);

	for (i = 0, j = 0; i < numlumps; i++) {
		if ((i < insert_start || i >= insert_end)
		 && VFS_SetHas(&p->tagged, dir[i].serial_no)) {
			// This is a lump we want to move. Swap it into its
			// new position.
			W_SwapEntries(wf, i, insert_point);
			++insert_point;
			// There is now an empty "unnamed" lump in the old
			// position, but it will be replaced the code below
			// that moves lumps back.
		} else {
			W_SwapEntries(wf, i, j);
			if (i == insert_point) {
				insert_point = j;
			}
			j++;
		}
	}

	// All of the empty unnamed lumps are now at the end of the directory,
	// and we can delete them.
	W_DeleteEntries(wf, numlumps - p->tagged.num_entries,
	                p->tagged.num_entries);
	W_CommitChanges(wf);

	UI_ListPaneSelect(&p->pane, insert_start + 1);
	VFS_Refresh(p->dir);
}

static void PerformRearrange(struct directory_pane *active_pane,
                             struct directory_pane *other_pane)
{
	if (active_pane->tagged.num_entries == 0) {
		UI_MessageBox("You have not selected any lumps to move.");
		return;
	}
	MoveLumps(active_pane, VFS_WadFile(active_pane->dir));
}

const struct action rearrange_action = {
	KEY_F(2), 'V', "Rearr", "Move (rearrange)",
	PerformRearrange,
};

static int CompareLumps(struct wad_file_entry *dir, unsigned int i1,
                        unsigned int i2)
{
	int cmp = strncmp(dir[i1].name, dir[i2].name, 8);

	// Fallback comparison preserves existing lump order.
	if (cmp == 0) {
		return i1 - i2;
	} else {
		return cmp;
	}
}

static void SortLumps(struct wad_file *wf, struct wad_file_entry *dir,
                      unsigned int *indexes, unsigned int count)
{
	unsigned int i, pivot;

	if (count < 2) {
		return;
	}

	// Pick pivot from middle (to avoid degenerate case if already sorted,
	// and swap it to the start.
	pivot = count / 2;
	W_SwapEntries(wf, indexes[pivot], indexes[0]);
	pivot = 0;

	for (i = 1; i < count; i++) {
		if (CompareLumps(dir, indexes[i], indexes[pivot]) < 0) {
			// This belongs before the pivot.
			W_SwapEntries(wf, indexes[i], indexes[pivot]);
			++pivot;
			// Swap back pivot into place
			W_SwapEntries(wf, indexes[i], indexes[pivot]);
		}
	}

	SortLumps(wf, dir, indexes, pivot);
	SortLumps(wf, dir, indexes + pivot + 1, count - pivot - 1);
}

static void PerformSortLumps(struct directory_pane *active_pane,
                             struct directory_pane *other_pane)
{
	struct wad_file *wf;
	struct wad_file_entry *dir;
	unsigned int *indexes;
	int i, j, num_lumps, num_tagged;

	num_tagged = active_pane->tagged.num_entries;
	if (num_tagged ==  0) {
		UI_MessageBox("You have not selected anything to sort.");
		return;
	}

	wf = VFS_WadFile(active_pane->dir);
	num_lumps = W_NumLumps(wf);
	dir = W_GetDirectory(wf);

	// Build array of lump indexes for each tagged item.
	indexes = checked_calloc(num_tagged, sizeof(int));
	for (i = 0, j = 0; i < num_lumps; i++) {
		if (VFS_SetHas(&active_pane->tagged, dir[i].serial_no)) {
			indexes[j] = i;
			++j;
		}
	}

	// Check if already sorted.
	for (i = 0; i < num_tagged - 1; i++) {
		if (CompareLumps(dir, indexes[i], indexes[i + 1]) > 0) {
			break;
		}
	}

	if (i < num_tagged - 1) {
		SortLumps(wf, dir, indexes, num_tagged);
	} else if (UI_ConfirmDialogBox("Sort lumps", "Sort", "Cancel",
	                               "Lumps already sorted.\n"
	                               "Sort into reverse order?")) {
		// Reverse sort doesn't even require using SortLumps(). The
		// lumps are already sorted, so we just need to reverse
		// them.
		for (i = 0; i < num_tagged / 2; i++) {
			W_SwapEntries(wf, indexes[i],
			              indexes[num_tagged - i - 1]);
		}
	}

	free(indexes);

	W_CommitChanges(wf);
	VFS_Refresh(active_pane->dir);
}

const struct action sort_lumps_action = {
	SHIFT_KEY_F(2), 0, "Sort", "Sort lumps",
	PerformSortLumps,
};

static void PerformNewLump(struct directory_pane *active_pane,
                           struct directory_pane *other_pane)
{
	int selected = UI_DirectoryPaneSelected(active_pane);
	struct wad_file *f = VFS_WadFile(active_pane->dir);

	char *name = UI_TextInputDialogBox(
		"New lump", "Create", 8,
		"Enter name for new lump:");
	if (name == NULL) {
		return;
	}
	// TODO: Should we be creating through VFS?
	W_AddEntries(f, selected + 1, 1);
	W_SetLumpName(f, selected + 1, name);
	free(name);
	W_CommitChanges(f);
	VFS_Refresh(active_pane->dir);
	UI_ListPaneKeypress(active_pane, KEY_DOWN);
}

const struct action new_lump_action = {
	KEY_F(7), 'K', "NewLump", ". New lump",
	PerformNewLump,
};

static void PerformRename(struct directory_pane *active_pane,
                          struct directory_pane *other_pane)
{
	char *input_filename;
	struct file_set *tagged = UI_DirectoryPaneTagged(active_pane);
	int selected = UI_DirectoryPaneSelected(active_pane);
	char *old_name = active_pane->dir->entries[selected].name;
	uint64_t serial_no = active_pane->dir->entries[selected].serial_no;

	if (tagged->num_entries == 0) {
		UI_MessageBox(
		    "You have not selected anything to rename.");
		return;
	} else if (tagged->num_entries > 1) {
		UI_MessageBox(
		    "You can't rename more than one thing at once.");
		return;
	}

	input_filename = UI_TextInputDialogBox(
	    "Rename", "Rename", 30, "New name for '%s'?", old_name);
	if (input_filename == NULL) {
		return;
	}
	VFS_Rename(active_pane->dir, &active_pane->dir->entries[selected],
		   input_filename);
	VFS_CommitChanges(active_pane->dir);
	VFS_Refresh(active_pane->dir);
	free(input_filename);
	UI_DirectoryPaneSelectBySerial(active_pane, serial_no);
}

const struct action rename_action = {
	KEY_F(6), 'B', "Ren", ". Rename",
	PerformRename,
};

static void PerformDelete(struct directory_pane *active_pane,
                          struct directory_pane *other_pane)
{
	struct file_set *tagged = UI_DirectoryPaneTagged(active_pane);
	char buf[64];
	int i;

	if (tagged->num_entries == 0) {
		UI_MessageBox(
		    "You have not selected anything to delete.");
		return;
	}

	VFS_DescribeSet(active_pane->dir, tagged, buf, sizeof(buf));
	if (!UI_ConfirmDialogBox("Confirm Delete", "Delete", "Cancel",
	                         "Delete %s?", buf)) {
		return;
	}
	// Note that there's a corner-case gotcha here. VFS serial
	// numbers for files are inode numbers, and through hardlinks
	// multiple files can have the same inode number. However,
	// the way things are implemented here, we only ever delete one
	// of each serial number. So the wrong file can end up being
	// deleted, but we'll never delete both.
	for (i = 0; i < tagged->num_entries; i++) {
		struct directory_entry *ent;
		ent = VFS_EntryBySerial(active_pane->dir, tagged->entries[i]);
		if (ent == NULL) {
			continue;
		}
		VFS_Remove(active_pane->dir, ent);
	}
	VFS_CommitChanges(active_pane->dir);
	VFS_ClearSet(&active_pane->tagged);
	VFS_Refresh(active_pane->dir);
	if (UI_DirectoryPaneSelected(active_pane)
	    >= active_pane->dir->num_entries) {
		UI_ListPaneSelect(&active_pane->pane,
		                  active_pane->dir->num_entries);
	}
}

const struct action delete_action = {
	KEY_F(8), 'X', "Del", "Delete",
	PerformDelete,
};

static void PerformMarkPattern(struct directory_pane *active_pane,
                               struct directory_pane *other_pane)
{
	int first_match;
	char *glob = UI_TextInputDialogBox(
		"Mark pattern", "Mark", 15,
		"Enter a wildcard pattern (eg. *.png):");
	if (glob == NULL) {
		return;
	}
	first_match = VFS_AddGlobToSet(active_pane->dir, &active_pane->tagged, glob);
	if (first_match < 0) {
		UI_MessageBox("No matches found.");
	} else {
		UI_ListPaneSelect(&active_pane->pane, first_match + 1);
	}
	free(glob);
}

const struct action mark_pattern_action = {
	0, 'G', "MarkPat", ". Mark pattern",
	PerformMarkPattern,
};

static void PerformUnmarkAll(struct directory_pane *active_pane,
                             struct directory_pane *other_pane)
{
	VFS_ClearSet(&active_pane->tagged);
}

const struct action unmark_all_action = {
	KEY_F(10), 'A', "UnmrkAll", "Unmark all",
	PerformUnmarkAll,
};

static void PerformMark(struct directory_pane *active_pane,
                        struct directory_pane *other_pane)
{
	int selected = UI_DirectoryPaneSelected(active_pane);
	struct directory_entry *ent;

	if (active_pane->pane.selected <= 0) {
		return;
	}
	ent = &active_pane->dir->entries[selected];
	if (VFS_SetHas(&active_pane->tagged, ent->serial_no)) {
		VFS_RemoveFromSet(&active_pane->tagged, ent->serial_no);
	} else {
		VFS_AddToSet(&active_pane->tagged, ent->serial_no);
	}
	UI_ListPaneKeypress(active_pane, KEY_DOWN);
}

const struct action mark_action = {
	' ', 0, "Un/mark", "Mark/unmark",
	PerformMark,
};

// Called when closing a file to check if it needs cleaning out.
static void CheckCompactWad(struct directory_pane *pane)
{
	struct wad_file *wf = VFS_WadFile(pane->dir);
	const char *filename;
	uint32_t junk_bytes_kb;

	if (wf == NULL) {
		return;
	}

	// Has file been changed?
	if (W_CanUndo(wf) == 0 && W_CanRedo(wf) == 0) {
		return;
	}
	// Insignificant amount of junk?
	junk_bytes_kb = W_NumJunkBytes(wf) / 1000;
	if (junk_bytes_kb < WAD_JUNK_THRESHOLD_KB) {
		return;
	}
	filename = PathBaseName(pane->dir->path);
	if (!UI_ConfirmDialogBox(
		"Compact WAD", "Compact", "Ignore",
		"'%s' contains %dKB of junk data.\nCompact now?",
		filename, junk_bytes_kb)) {
		return;
	}
	if (!W_CompactWAD(wf)) {
		UI_MessageBox("Error when compacting '%s'.", filename);
	}
}

static void PerformQuit(struct directory_pane *active_pane,
                        struct directory_pane *other_pane)
{
	// Editing may have left some junk data. Prompt to see if
	// the user wants to clean it out.
	CheckCompactWad(active_pane);
	CheckCompactWad(other_pane);
	UI_ExitMainLoop();
}

const struct action quit_action = {
	27, 'Q', "Quit", "Quit",
	PerformQuit,
};

static void RedrawScreen(struct directory_pane *a,
                         struct directory_pane *b)
{
	clearok(stdscr, TRUE);
	wrefresh(stdscr);
}

const struct action redraw_screen_action = {
        0, 'L', "Redraw", "Redraw screen",
	RedrawScreen,
};

static void PerformReload(struct directory_pane *active_pane,
                          struct directory_pane *other_pane)
{
	VFS_Refresh(active_pane->dir);
}

const struct action reload_action = {
	0, 'R', "Reload", "Reload",
	PerformReload,
};

static void NavigateNew(struct directory_pane *active_pane,
                        struct directory_pane *other_pane)
{
	struct directory_pane *new_pane = NULL;
	struct directory *new_dir;
	const char *old_path;
	char *path;

	path = UI_DirectoryPaneEntryPath(active_pane);

	// Don't allow the same WAD to be opened twice.
	if (!strcmp(path, other_pane->dir->path)
	 && other_pane->dir->type == FILE_TYPE_WAD) {
		free(path);
		SwitchToPane(other_pane);
		return;
	}

	new_dir = VFS_OpenDir(path);
	if (new_dir == NULL) {
		int idx = UI_DirectoryPaneSelected(active_pane);
		free(path);
		UI_MessageBox("Error when opening '%s'.",
		              active_pane->dir->entries[idx].name);
		return;
	}

	new_pane = UI_NewDirectoryPane(NULL, new_dir);

	// Select subfolder we just navigated out of?
	old_path = active_pane->dir->path;
	if (strlen(path) < strlen(old_path)) {
		UI_DirectoryPaneSearch(new_pane, PathBaseName(old_path));
	}

	free(path);

	if (new_pane != NULL) {
		// We're closing the current pane; if it is a WAD we might
		// want to clean out any junk data we left behind.
		CheckCompactWad(active_pane);
		ReplacePane(active_pane, new_pane);
		SwitchToPane(new_pane);
	}
}

static void PerformView(struct directory_pane *active_pane,
                        struct directory_pane *other_pane)
{
	enum file_type typ;
	int selected;

	typ = UI_DirectoryPaneEntryType(active_pane);

	// Change directory?
	if (typ == FILE_TYPE_WAD || typ == FILE_TYPE_DIR) {
		NavigateNew(active_pane, other_pane);
		return;
	}

	selected = UI_DirectoryPaneSelected(active_pane);
	if (selected < 0) {
		return;
	}

	OpenDirent(active_pane->dir, &active_pane->dir->entries[selected]);
}

const struct action view_action = {
	'\r', 0,  "View", "View",
	PerformView,
};

static void PerformCompact(struct directory_pane *active_pane,
                           struct directory_pane *other_pane)
{
	const struct directory_entry *ent;
	struct wad_file *wf;
	bool already_open;
	char *path;
	enum file_type typ;
	uint32_t junk_bytes;
	int selected;

	selected = UI_DirectoryPaneSelected(active_pane);
	if (selected < 0) {
		return;
	}

	ent = &active_pane->dir->entries[selected];
	typ = UI_DirectoryPaneEntryType(active_pane);

	// Change directory?
	if (typ != FILE_TYPE_WAD) {
		UI_MessageBox("Can't compact; '%s' is not a WAD file.",
		              ent->name);
		return;
	}

	path = UI_DirectoryPaneEntryPath(active_pane);

	// WAD file may be open in the other pane; don't open twice.
	already_open = other_pane->dir->type == FILE_TYPE_WAD
	            && !strcmp(path, other_pane->dir->path);
	if (already_open) {
		wf = VFS_WadFile(other_pane->dir);
	} else {
		wf = W_OpenFile(path);
	}
	free(path);

	if (wf == NULL) {
		UI_MessageBox("Failed to open '%s'.", ent->name);
		return;
	}

	junk_bytes = W_NumJunkBytes(wf);
	if (junk_bytes == 0) {
		UI_MessageBox("'%s' cannot be made any smaller.", ent->name);
		goto fail;
	} else if (!UI_ConfirmDialogBox("Compact WAD",
		"Compact", "Cancel",
		"'%s' contains %d junk bytes.\nCompact WAD?",
		ent->name, junk_bytes)) {
		goto fail;
	}

	if (!W_CompactWAD(wf)) {
		UI_MessageBox("Failed to compact '%s'.", ent->name);
	}

	VFS_Refresh(active_pane->dir);
	VFS_Refresh(other_pane->dir);

fail:
	if (!already_open) {
		W_CloseFile(wf);
	}
}

const struct action compact_action = {
	KEY_F(2), 'P',  "Compact", "Compact WAD file",
	PerformCompact,
};

// TODO:
const struct action edit_action = {
	KEY_F(4), 'E', "Edit", "Edit",
};

const struct action hexdump_action = {
	0, 'D', "Hexdump", "| Hexdump",
};

static void PerformUndo(struct directory_pane *active_pane,
                        struct directory_pane *other_pane)
{
	struct wad_file *wf = VFS_WadFile(active_pane->dir);

	if (W_CanUndo(wf) == 0) {
		UI_MessageBox("There is nothing to undo.");
		return;
	}

	if (!W_Undo(wf, 1)) {
		UI_MessageBox("Undo failed.");
		return;
	}
	VFS_Refresh(active_pane->dir);
}

const struct action undo_action = {
	0, 'Z', "Undo", "Undo",
	PerformUndo,
};

static void PerformRedo(struct directory_pane *active_pane,
                        struct directory_pane *other_pane)
{
	struct wad_file *wf = VFS_WadFile(active_pane->dir);

	if (W_CanRedo(wf) == 0) {
		UI_MessageBox("There is nothing to redo.");
		return;
	}

	if (!W_Redo(wf, 1)) {
		UI_MessageBox("Redo failed.");
		return;
	}
	VFS_Refresh(active_pane->dir);
}

const struct action redo_action = {
	0, 'Y', "Redo", "| Redo",
	PerformRedo,
};
