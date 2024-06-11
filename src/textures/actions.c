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
#include "stringlib.h"
#include "ui/dialog.h"
#include "ui/ui.h"
#include "view.h"

#include "textures/textures.h"
#include "textures/internal.h"

extern void SwitchToPane(struct directory_pane *pane); // in wadgadget.c

static bool CheckExistingTexture(struct textures *txs, const char *name)
{
	bool existing = TX_TextureForName(txs, name) >= 0;

	if (existing) {
		UI_MessageBox("There is already a texture with this name.");
	}

	return !existing;
}

static void PerformNewTexture(struct directory_pane *active_pane,
                              struct directory_pane *other_pane)
{
	int pos = UI_DirectoryPaneSelected(active_pane) + 1;
	struct textures *txs = TX_TextureList(active_pane->dir);
	struct texture t;
	char *name;

	if (!CheckReadOnly(active_pane->dir)) {
		return;
	}

	if (pos < 1
	 && txs->num_textures > 0
	 && StringHasPrefix(txs->textures[0]->name, "AA")
	 && StringHasSuffix(active_pane->dir->path, "/TEXTURE1")
	 && !UI_ConfirmDialogBox("New texture", "Create here", "Cancel",
	                         "You are trying to insert a new texture\n"
	                         "before the '%.8s' dummy texture. This\n"
	                         "needs to be the first in the list, or\n"
	                         "your new texture will not work properly.\n"
	                         "\nAre you sure you want to proceed?",
	                         txs->textures[0]->name)) {
		return;
	}

	name = UI_TextInputDialogBox("New texture", "Create", 8,
	                             "Enter name for new texture:");
	if (name == NULL) {
		return;
	}

	if (!CheckExistingTexture(txs, name)) {
		free(name);
		return;
	}

	memset(&t, 0, sizeof(t));
	strncpy(t.name, name, 8);
	t.width = 128;
	t.height = 128;
	TX_AddTexture(txs, pos, &t);
	VFS_CommitChanges(active_pane->dir, "creation of texture '%s'", name);
	free(name);
	VFS_Refresh(active_pane->dir);
	UI_ListPaneKeypress(active_pane, KEY_DOWN);
}

const struct action new_texture_action = {
	KEY_F(7), 'K', "NewTxt", ". New texture",
	PerformNewTexture,
};

static void PerformEditConfig(struct directory_pane *active_pane,
                              struct directory_pane *other_pane)
{
	struct directory_revision *old_rev;
	struct directory *parent;
	struct directory_entry *ent;

	if (!TX_DirSave(active_pane->dir)) {
		UI_MessageBox("Failed to save directory for editing.");
		return;
	}

	parent = TX_DirGetParent(active_pane->dir, &ent);
	old_rev = parent->curr_revision;
	OpenDirent(parent, ent);

	// Editing may have reset the readonly flag.
	active_pane->dir->readonly = parent->readonly;

	// If the current revision of the WAD has changed, it can only be
	// because `OpenDirent()` imported a new version of the lump. So
	// detect this case and import it into the current directory. We
	// can then undo the edit to the WAD, so that it only takes effect
	// once exiting the current directory.
	if (!parent->readonly && parent->curr_revision != old_rev
	 && TX_DirReload(active_pane->dir)) {
		VFS_CommitChanges(active_pane->dir, "edit via text editor");
		VFS_ClearSet(&active_pane->tagged);
		VFS_Undo(parent, 1);
	}
}

const struct action edit_textures_action = {
	KEY_F(4), 'E', "EditCfg", "Edit texture config",
	PerformEditConfig,
};

const struct action edit_pnames_action = {
	KEY_F(4), 'E', "EditCfg", "Edit PNAMES config",
	PerformEditConfig,
};

static void PerformDuplicateTexture(struct directory_pane *active_pane,
                                    struct directory_pane *other_pane)
{
	struct textures *txs = TX_TextureList(active_pane->dir);
	struct file_set *tagged = UI_DirectoryPaneTagged(active_pane);
	struct texture *t;
	int idx = UI_DirectoryPaneSelected(active_pane);
	char *name;

	if (!CheckReadOnly(active_pane->dir)) {
		return;
	}

	if (tagged->num_entries != 1) {
		UI_MessageBox("You can only duplicate a single texture.");
		return;
	}
	if (idx < 0 || idx >= txs->num_textures) {
		UI_MessageBox("You have not selected a texture.");
		return;
	}

	name = UI_TextInputDialogBox("Duplicate texture", "Duplicate", 8,
	                             "Enter name for new texture:");
	if (name == NULL) {
		return;
	}

	if (!CheckExistingTexture(txs, name)) {
		free(name);
		return;
	}

	t = TX_DupTexture(txs->textures[idx]);
	strncpy(t->name, name, 8);

	TX_AddTexture(txs, idx + 1, t);
	free(t);

	VFS_CommitChanges(active_pane->dir, "creation of texture '%s'", name);
	free(name);
	VFS_Refresh(active_pane->dir);
	UI_ListPaneKeypress(active_pane, KEY_DOWN);
}

const struct action dup_texture_action = {
	KEY_F(3), 'U', "DupTxt", ". Duplicate texture",
	PerformDuplicateTexture,
};

static void PerformExportConfig(struct directory_pane *active_pane,
                                struct directory_pane *other_pane)
{
	struct file_set *selected;
	char *filename = NULL, *filename2 = NULL;
	VFILE *formatted, *out_wrapped;
	FILE *out;

	if (active_pane->tagged.num_entries > 0) {
		selected = &active_pane->tagged;
	} else if (!UI_ConfirmDialogBox(
	                "Export config", "Export", "Cancel",
	                "You have not selected any textures to\n"
	                "export. Export the entire directory?")) {
		return;
	} else {
		selected = NULL;
	}

	formatted = TX_DirFormatConfig(active_pane->dir, selected);
	if (formatted == NULL) {
		return;
	}

	filename = UI_TextInputDialogBox(
		"Export config",
		"Export", 30, "Enter filename to export:");

	if (filename == NULL) {
		goto cancel;
	}

	if (VFS_EntryByName(other_pane->dir, filename) != NULL
	 && !UI_ConfirmDialogBox("Confirm Overwrite", "Overwrite", "Cancel",
	                         "Overwrite existing '%s'?", filename)) {
		goto cancel;
	}

	filename2 = StringJoin("/", other_pane->dir->path, filename, NULL);

	// TODO: This should be written through VFS.
	out = fopen(filename2, "w");
	if (out == NULL) {
		UI_MessageBox("Failed to open file for write:\n%s",
		              filename2);
		vfclose(formatted);
		goto cancel;
	}

	out_wrapped = vfwrapfile(out);
	vfcopy(formatted, out_wrapped);
	vfclose(formatted);
	vfclose(out_wrapped);

	VFS_Refresh(other_pane->dir);

	SwitchToPane(other_pane);
	UI_DirectoryPaneSelectByName(other_pane, filename);

cancel:
	free(filename);
	free(filename2);
}

const struct action export_texture_config = {
	KEY_F(5), 'O', "ExpCfg", "> Export config",
	PerformExportConfig,
};

static void PerformNewPname(struct directory_pane *active_pane,
                            struct directory_pane *other_pane)
{
	struct pnames *pn = TX_GetDirPnames(active_pane->dir);
	int idx;
	char *name;

	if (!CheckReadOnly(active_pane->dir)) {
		return;
	}

	name = UI_TextInputDialogBox("New pname", "Create", 8,
	                             "Enter new patch name:");
	if (name == NULL) {
		return;
	}

	if (TX_GetPnameIndex(pn, name) >= 0) {
		UI_DirectoryPaneSelectByName(active_pane, name);
		UI_MessageBox("'%s' is already in the list.");
		free(name);
		return;
	}

	// We always add the new pname to the end of the directory, as putting
	// it in the middle of the directory screws up the other indexes.
	// That's not to say that we don't allow it to be subsequently moved
	// into a different position, but just adding a pname always does
	// something safe.
	idx = TX_AppendPname(pn, name);
	VFS_CommitChanges(active_pane->dir, "creation of pname '%s'", name);
	VFS_Refresh(active_pane->dir);
	UI_DirectoryPaneSelectEntry(active_pane,
	                            &active_pane->dir->entries[idx]);
	free(name);
}

const struct action new_pname_action = {
	KEY_F(7), 'K', "NewPname", ". New pname",
	PerformNewPname,
};

static void PnamesCopyNotice(struct directory *from_dir,
                             struct directory *to_dir,
                             struct file_set *copied, struct file_set *unused)
{
	char copied_descr[64] = "", unused_descr[64] = "";
	const char *sep;

	if (copied->num_entries > 0) {
		VFS_DescribeSet(to_dir, copied, copied_descr,
		                sizeof(copied_descr));
		StringConcat(copied_descr, " copied", sizeof(copied_descr));
	}
	if (unused->num_entries > 0) {
		VFS_DescribeSet(from_dir, unused, unused_descr,
		                sizeof(unused_descr));
		StringConcat(unused_descr, " already present",
		             sizeof(unused_descr));
	}

	if (strlen(copied_descr) > 0 && strlen(unused_descr) > 0) {
		sep = "; ";
	} else {
		sep = "";
	}
	UI_ShowNotice("%s%s%s.", copied_descr, sep, unused_descr);
}

static void PerformCopyPnames(struct directory_pane *active_pane,
                              struct directory_pane *other_pane)
{
	struct file_set *tagged = UI_DirectoryPaneTagged(active_pane);
	struct file_set copied = EMPTY_FILE_SET, unused = EMPTY_FILE_SET;
	struct directory *from_dir = active_pane->dir,
	                 *to_dir = other_pane->dir;
	struct pnames *pn = TX_GetDirPnames(other_pane->dir);
	struct directory_entry *ent;
	int idx;

	if (tagged->num_entries == 0) {
		UI_MessageBox("You have not selected any pnames to copy.");
		return;
	}

	if (!CheckReadOnly(to_dir)) {
		return;
	}

	idx = 0;
	while ((ent = VFS_IterateSet(from_dir, tagged, &idx)) != NULL) {
		if (TX_GetPnameIndex(pn, ent->name) >= 0) {
			VFS_AddToSet(&unused, TX_PnameSerialNo(ent->name));
			continue;
		}
		TX_AppendPname(pn, ent->name);
		VFS_AddToSet(&copied, TX_PnameSerialNo(ent->name));
	}

	VFS_Refresh(to_dir);

	PnamesCopyNotice(from_dir, to_dir, &copied, &unused);

	if (copied.num_entries > 0) {
		char buf[32];

		VFS_DescribeSet(to_dir, &copied, buf, sizeof(buf));
		VFS_CommitChanges(to_dir, "copy of %s", buf);

		VFS_ClearSet(&active_pane->tagged);
		UI_DirectoryPaneSetTagged(other_pane, &copied);
		SwitchToPane(other_pane);
	}

	VFS_FreeSet(&copied);
	VFS_FreeSet(&unused);
}

const struct action copy_pnames_action = {
	KEY_F(5), 'O', "Copy", "> Copy names",
	PerformCopyPnames,
};
