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
#include <stdbool.h>

#include "ui/actions_bar.h"
#include "ui/dialog.h"
#include "ui/title_bar.h"

#include "browser/actions.h"
#include "browser/browser.h"
#include "browser/directory_pane.h"
#include "common.h"
#include "conv/error.h"
#include "conv/export.h"
#include "stringlib.h"
#include "view.h"
#include "fs/wad_file.h"

#include "palette/palette.h"
#include "palette/palfs.h"

static void PerformViewPalette(void)
{
	struct directory_entry *ent = B_DirectoryPaneEntry(active_pane);

	if (ent->type == FILE_TYPE_PALETTE) {
		OpenDirent(PAL_InnerDir(active_pane->dir),
		           PAL_InnerEntry(active_pane->dir, ent), false);
		return;
	}

	// Otherwise, do the normal "View" action thing:
	view_action.callback();
}

const struct action view_palette_action = {
	'\r', 0,  "View", "View",
	PerformViewPalette,
};

static void PerformSetDefault(void)
{
	struct directory_entry *ent = B_DirectoryPaneEntry(active_pane);

	if (ent->type != FILE_TYPE_PALETTE) {
		return;
	}

	ent = PAL_InnerEntry(active_pane->dir, ent);

	PAL_SetDefaultPointer(ent->name);
	VFS_Refresh(active_pane->dir);
}

const struct action set_default_palette_action = {
	KEY_F(4), 'D',  "SetDef", "Set default",
	PerformSetDefault,
};

static struct palette_set *LoadPalette(struct directory *dir,
                                       struct directory_entry *ent)
{
	VFILE *input = VFS_OpenByEntry(dir, ent);
	const struct lump_type *lt;

	if (input == NULL) {
		return NULL;
	}

	switch (ent->type) {
	case FILE_TYPE_FILE:
		return PAL_FromImageFile(input);

	case FILE_TYPE_LUMP:
		lt = LI_IdentifyLump(VFS_WadFile(dir), ent - dir->entries);
		if (lt != &lump_type_palette) {
			ConversionError("%s lump is not a palette", ent->name);
			vfclose(input);
			return NULL;
		}
		return PAL_UnmarshalPaletteSet(input);

	default:
		return NULL;
	}
}

static struct directory *ActualDir(struct directory *dir)
{
	if (dir->type == FILE_TYPE_PALETTES) {
		return PAL_InnerDir(dir);
	}

	return dir;
}

static bool CopyPaletteToWAD(struct directory *from,
                             struct directory_entry *ent, struct wad_file *wf,
                             struct file_set *result)
{
	struct palette_set *set = LoadPalette(from, ent);
	VFILE *converted, *out;
	int idx;

	if (set == NULL) {
		// TODO
		return false;
	}
	if (set->num_palettes < 2) {
		ConversionError("%s only contains a single palette, which "
		                "is not\nenough for a complete PLAYPAL lump. "
		                "You should\nmaybe copy a PLAYPAL lump from "
		                "another WAD.", ent->name);
		return false;
	}

	converted = PAL_MarshalPaletteSet(set);
	idx = W_NumLumps(wf);
	W_AddEntries(wf, idx, 1);
	W_SetLumpName(wf, idx, "PLAYPAL");
	out = W_OpenLumpRewrite(wf, idx);
	vfcopy(converted, out);
	vfclose(converted);
	vfclose(out);

	VFS_AddToSet(result, W_GetDirectory(wf)[idx].serial_no);
	PAL_FreePaletteSet(set);

	return true;
}

static void PerformPaletteCopyToWAD(void)
{
	struct directory *from = ActualDir(active_pane->dir);
	struct file_set *set = B_DirectoryPaneTagged(active_pane);
	struct wad_file *wf = VFS_WadFile(other_pane->dir);
	char buf[32];
	struct directory_entry *ent;
	struct file_set result = EMPTY_FILE_SET;
	bool success = true;
	int idx = 0;

	if (set->num_entries < 1) {
		return;
	} else if (set->num_entries > 1
	        && !UI_ConfirmDialogBox("Confirm", "Proceed", "Cancel",
	                                "Create multiple PLAYPAL lumps?")) {
		return;
	}

	if (!B_CheckReadOnly(other_pane->dir)) {
		return;
	}

	ClearConversionErrors();
	while (success && (ent = VFS_IterateSet(from, set, &idx)) != NULL) {
		success = CopyPaletteToWAD(from, ent, wf, &result);
	}

	if (!success) {
		VFS_Rollback(other_pane->dir);
		UI_MessageBox("Import to WAD failed:\n%s",
		              GetConversionError());
		VFS_FreeSet(&result);
		return;
	}

	VFS_DescribeSet(from, set, buf, sizeof(buf));
	VFS_CommitChanges(other_pane->dir, "import of %s", buf);
	VFS_Refresh(other_pane->dir);

	B_DirectoryPaneSetTagged(other_pane, &result);
	B_SwitchToPane(other_pane);
	VFS_FreeSet(&result);
}

const struct action copy_palette_to_wad_action = {
	KEY_F(5), 'C',  "Copy", "> Copy",
	PerformPaletteCopyToWAD,
};

static bool CopyPaletteToDir(struct directory *from,
                             struct directory_entry *ent, struct directory *to,
                             struct file_set *result)
{
	struct palette_set *set = LoadPalette(from, ent);
	struct directory_entry *to_ent;
	VFILE *converted, *out;
	char *filename, *full_path;

	if (set == NULL) {
		// TODO
		return false;
	}

	if (from->type == FILE_TYPE_WAD) {
		filename = StringJoin("", PathBaseName(from->path),
		                      " palette.png", NULL);
	} else {
		filename = checked_strdup(PathBaseName(ent->name));
	}

	converted = PAL_ToImageFile(set);
	full_path = StringJoin("/", to->path, filename, NULL);
	out = vfwrapfile(fopen(full_path, "wb"));
	assert(out != NULL); // TODO
	free(full_path);
	vfcopy(converted, out);
	vfclose(converted);
	vfclose(out);

	PAL_FreePaletteSet(set);

	VFS_Refresh(to);
	to_ent = VFS_EntryByName(to, filename);
	assert(to_ent != NULL);
	VFS_AddToSet(result, to_ent->serial_no);

	free(filename);

	return true;
}

static void PerformPaletteCopyToDir(void)
{
	struct directory *from = ActualDir(active_pane->dir),
	                 *to = ActualDir(other_pane->dir);
	struct file_set *set = B_DirectoryPaneTagged(active_pane);
	struct file_set result = EMPTY_FILE_SET;
	struct directory_entry *ent;
	bool success = true;
	int idx = 0;

	ClearConversionErrors();
	while (success && (ent = VFS_IterateSet(from, set, &idx)) != NULL) {
		success = CopyPaletteToDir(from, ent, to, &result);
	}

	if (!success) {
		UI_MessageBox("Copy failed:\n%s", GetConversionError());
		VFS_FreeSet(&result);
		return;
	}

	VFS_Refresh(other_pane->dir);
	B_DirectoryPaneSetTagged(other_pane, &result);
	B_SwitchToPane(other_pane);
	VFS_FreeSet(&result);
}

const struct action copy_palette_to_dir_action = {
	KEY_F(5), 'C',  "Copy", "> Copy",
	PerformPaletteCopyToDir,
};

static void PerformSetPalettePref(void)
{
	struct file_set *set = B_DirectoryPaneTagged(active_pane);
	struct wad_file *wf = VFS_WadFile(other_pane->dir);
	VFILE *in, *out, *marshaled;
	struct palette_set *pal;
	struct directory_entry *ent;
	int idx = 0;

	if (set->num_entries != 1) {
		UI_MessageBox("You must select a single palette.");
		return;
	}

	if (!B_CheckReadOnly(other_pane->dir)) {
		return;
	}

	ent = VFS_EntryBySerial(active_pane->dir, set->entries[0]);
	if (ent == NULL) {
		return;
	}

	in = VFS_OpenByEntry(active_pane->dir, ent);
	if (in == NULL) {
		return;
	}

	ClearConversionErrors();
	pal = PAL_FromImageFile(in);
	if (pal == NULL) {
		UI_MessageBox("Error loading palette:\n%s",
		              GetConversionError());
		return;
	}

	// We only include the first palette.
	pal->num_palettes = 1;
	marshaled = PAL_MarshalPaletteSet(pal);
	PAL_FreePaletteSet(pal);

	// Write lump. If there's an existing lump we overwrite it.
	idx = W_GetNumForName(wf, "PALPREF");
	if (idx < 0) {
		idx = W_NumLumps(wf);
		W_AddEntries(wf, idx, 1);
		W_SetLumpName(wf, idx, "PALPREF");
	}
	out = W_OpenLumpRewrite(wf, idx);
	vfcopy(marshaled, out);
	vfclose(marshaled);
	vfclose(out);

	VFS_CommitChanges(other_pane->dir, "setting preferred palette");
	VFS_Refresh(other_pane->dir);
	B_DirectoryPaneSelectByName(other_pane, "PALPREF");
	B_SwitchToPane(other_pane);
}

const struct action set_palette_pref_action = {
	KEY_F(3), 'U',  "SetPref", "> Use for WAD",
	PerformSetPalettePref,
};
