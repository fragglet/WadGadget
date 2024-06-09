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
#include "textures/textures.h"
#include "ui/dialog.h"
#include "view.h"

extern void SwitchToPane(struct directory_pane *pane); // in wadgadget.c

static bool CheckExistingTexture(struct textures *txs, const char *name)
{
	bool existing = TX_TextureForName(txs, name) != NULL;

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

static void PerformEditTextures(struct directory_pane *active_pane,
                                struct directory_pane *other_pane)
{
	struct directory *parent;
	struct directory_entry *ent;

	// TODO: Write current state of the texture dir before edit
	parent = TX_DirGetParent(active_pane->dir, &ent);
	OpenDirent(parent, ent);

	TX_DirReload(active_pane->dir);
	VFS_ClearSet(&active_pane->tagged);
}

const struct action edit_textures_action = {
	KEY_F(4), 'E', "EditCfg", "Edit texture config",
	PerformEditTextures,
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

static struct textures *MakeTextureSubset(struct textures *txs,
                                          struct file_set *fs)
{
	struct textures *result = checked_calloc(1, sizeof(struct textures));
	unsigned int i;

	for (i = 0; i < txs->num_textures; i++) {
		if (VFS_SetHas(fs, txs->serial_nos[i])) {
			TX_AddTexture(result, result->num_textures,
			              txs->textures[i]);
		}
	}

	return result;
}

static void PerformExportConfig(struct directory_pane *active_pane,
                                struct directory_pane *other_pane)
{
	FILE *out;
	VFILE *out_wrapped, *marshaled;
	char *filename = NULL, *filename2 = NULL;
	char comment_buf[32];
	struct directory *parent;
	struct textures *txs, *subset;
	struct pnames *pn;

	txs = TX_TextureList(active_pane->dir);
	parent = TX_DirGetParent(active_pane->dir, NULL);

	if (active_pane->tagged.num_entries > 0) {
		// Make a new subset texture directory and convert
		// it to a config text file.
		subset = MakeTextureSubset(txs, &active_pane->tagged);
	} else if (!UI_ConfirmDialogBox(
	                "Export texture config", "Export", "Cancel",
	                "You have not selected any textures to\n"
	                "export. Export the entire directory?")) {
		return;
	} else {
		subset = txs;
	}

	filename = UI_TextInputDialogBox(
		"Export texture config",
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

	pn = TX_GetDirPnames(active_pane->dir);
	snprintf(comment_buf, sizeof(comment_buf), "Exported from %s",
	         PathBaseName(parent->path));
	marshaled = TX_FormatTexturesConfig(subset, pn, comment_buf);

	// TODO: This should be written through VFS.
	out = fopen(filename2, "w");
	if (out == NULL) {
		UI_MessageBox("Failed to open file for write:\n%s",
		              filename2);
		vfclose(marshaled);
		goto cancel;
	}

	out_wrapped = vfwrapfile(out);
	vfcopy(marshaled, out_wrapped);
	vfclose(marshaled);
	vfclose(out_wrapped);

	VFS_Refresh(other_pane->dir);

	SwitchToPane(other_pane);
	UI_DirectoryPaneSelectByName(other_pane, filename);

cancel:
	if (subset != txs) {
		TX_FreeTextures(subset);
	}
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
