//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <assert.h>
#include <curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "browser/actions.h"
#include "browser/browser.h"
#include "browser/directory_pane.h"
#include "common.h"
#include "conv/error.h"
#include "fs/lump_dir.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "stringlib.h"
#include "textures/editor.h"
#include "textures/internal.h"
#include "textures/textures.h"
#include "ui/actions_bar.h"
#include "ui/dialog.h"
#include "ui/list_pane.h"
#include "ui/title_bar.h"
#include "view.h"

static struct textures *MakeTextureSubset(struct textures *txs,
                                          struct file_set *files)
{
	struct textures *result = TX_NewTextureList(0);
	unsigned int i;

	for (i = 0; i < txs->num_textures; i++) {
		if (files == NULL || VFS_SetHas(files, txs->serial_nos[i])) {
			TX_AddTexture(result, result->num_textures,
			              txs->textures[i]);
		}
	}

	return result;
}

static struct pnames *MakePnamesSubset(struct pnames *pn,
                                       struct file_set *files)
{
	struct pnames *result = TX_NewPnamesList(0);
	int i;

	for (i = 0; i < pn->num_pnames; i++) {
		if (files == NULL ||
		    VFS_SetHas(files, TX_PnameSerialNo(pn->pnames[i]))) {
			TX_AppendPname(result, pn->pnames[i]);
		}
	}

	return result;
}

static VFILE *FormatConfig(struct directory *dir, struct file_set *files)
{
	VFILE *result;

	if (dir->type == &file_type_texture_list) {
		struct texture_bundle *b = TX_DirGetBundle(dir);
		struct textures *txs = MakeTextureSubset(b->txs, files);
		char comment_buf[32];

		snprintf(comment_buf, sizeof(comment_buf), "Exported from %s",
		         PathBaseName(VFS_LumpDirGetParent(dir, NULL)->path));
		result = TX_FormatTexturesConfig(txs, b->pn, comment_buf);
		TX_FreeTextures(txs);
		return result;
	} else if (dir->type == &file_type_pnames_list) {
		struct pnames *pn = TX_PnamesList(dir);
		struct pnames *subset = MakePnamesSubset(pn, files);
		result = TX_FormatPnamesConfig(subset);
		TX_FreePnames(subset);
		return result;
	}
	assert(0);
}

static bool CheckExistingTexture(struct textures *txs, const char *name)
{
	bool existing = TX_TextureForName(txs, name) >= 0;

	if (existing) {
		UI_MessageBox("There is already a texture with this name.");
	}

	return !existing;
}

static void ActionNewTexture(void)
{
	int pos = B_DirectoryPaneSelected(active_pane) + 1;
	struct textures *txs = TX_TextureList(active_pane->dir);
	struct texture t;
	char *name;

	if (!B_CheckReadOnly(active_pane->dir)) {
		return;
	}

	if (pos < 1 && txs->num_textures > 0 &&
	    StringHasPrefix(txs->textures[0]->name, "AA") &&
	    StringHasSuffix(active_pane->dir->path,
	                    DIR_SEPARATOR_S "TEXTURE1") &&
	    !UI_ConfirmDialogBox("New texture", "Create here", "Cancel",
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
    KEY_F(7), 'K', "NewTxt", ". New texture", ActionNewTexture,
};

static void ActionEditConfig(void)
{
	struct directory *parent;
	struct directory_entry *ent;

	// A previous version of this action tried to do a more elaborate
	// dance of saving the lump, launching the editor, loading the
	// reimported saved version and undoing the WAD changes so that
	// we could save them from within the texture browser. It all
	// proved too complicated and fragile, so now instead we just do
	// the simplest thing that works: close the texture list and go
	// back to the WAD, then launch the editor. If the user wants the
	// texture list again they can just open it.
	parent = VFS_LumpDirGetParent(active_pane->dir, &ent);
	parent_dir_action.callback();

	OpenDirent(parent, ent, true);
}

const struct action edit_textures_action = {
    KEY_F(4), 'F', "EditCfg", "Edit texture config", ActionEditConfig,
};

const struct action edit_pnames_action = {
    KEY_F(4), 'F', "EditCfg", "Edit PNAMES config", ActionEditConfig,
};

static void ActionEditTexture(void)
{
	struct texture_bundle *b = TX_DirGetBundle(active_pane->dir);
	int tx_num = B_DirectoryPaneSelected(active_pane);

	if (tx_num == -1) {
		parent_dir_action.callback();
		return;
	}

	if (!B_CheckReadOnly(active_pane->dir)) {
		return;
	}

	if (TX_EditTexture(b, tx_num)) {
		VFS_CommitChanges(active_pane->dir, "edit to '%s'",
		                  b->txs->textures[tx_num]->name);
	} else {
		VFS_Rollback(active_pane->dir);
	}

	VFS_Refresh(active_pane->dir);
}

const struct action edit_texture_action = {
    '\r', 0, "Edit", "Edit texture", ActionEditTexture,
};

static void ActionDuplicateTexture(void)
{
	struct textures *txs = TX_TextureList(active_pane->dir);
	struct file_set *tagged = B_DirectoryPaneTagged(active_pane);
	struct texture *t;
	int idx = B_DirectoryPaneSelected(active_pane);
	char *name;

	if (tagged->num_entries != 1) {
		UI_MessageBox("You can only duplicate a single texture.");
		return;
	}
	if (idx < 0 || idx >= txs->num_textures) {
		UI_MessageBox("You have not selected a texture.");
		return;
	}

	if (!B_CheckReadOnly(active_pane->dir)) {
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
    KEY_F(3), 'U', "DupTxt", ". Duplicate texture", ActionDuplicateTexture,
};

static void ActionExportConfig(void)
{
	struct file_set *selected;
	char *filename = NULL, *filename2 = NULL;
	VFILE *formatted, *out;

	if (active_pane->tagged.num_entries > 0) {
		selected = &active_pane->tagged;
	} else if (UI_ConfirmDialogBox(
	               "Export config", "Export", "Cancel",
	               "You have not selected any textures to\n"
	               "export. Export the entire directory?")) {
		selected = NULL;
	} else {
		return;
	}

	formatted = FormatConfig(active_pane->dir, selected);
	if (formatted == NULL) {
		return;
	}

	filename = UI_TextInputDialogBox("Export config", "Export", 30,
	                                 "Enter filename to export:");

	if (filename == NULL) {
		goto cancel;
	}

	if (VFS_EntryByName(other_pane->dir, filename) != NULL &&
	    !UI_ConfirmDialogBox("Confirm Overwrite", "Overwrite", "Cancel",
	                         "Overwrite existing '%s'?", filename)) {
		goto cancel;
	}

	filename2 =
	    StringJoin(DIR_SEPARATOR_S, other_pane->dir->path, filename, NULL);

	// TODO: This should be written through VFS.
	out = vfwrapfile(fopen(filename2, "w"));
	if (out == NULL) {
		UI_MessageBox("Failed to open file for write:\n%s", filename2);
		vfclose(formatted);
		goto cancel;
	}

	vfcopy(formatted, out);
	vfclose(out);

	VFS_Refresh(other_pane->dir);

	B_DirectoryPaneSelectByName(other_pane, filename);
	B_SwitchToPane(other_pane);

cancel:
	vfclose(formatted);
	free(filename);
	free(filename2);
}

const struct action export_texture_config = {
    KEY_F(5), 'C', "ExpCfg", "> Export config", ActionExportConfig,
};

static void MergeTexturesResultNotice(struct texture_bundle_merge_result *r)
{
	char buf[64] = "";
	size_t buf_len = sizeof(buf), cnt;
	char *p = buf;

	if (r->textures_added + r->textures_overwritten == 0) {
		UI_ShowNotice("No new or changed textures added.");
		return;
	}

	if (r->textures_added > 0) {
		cnt = snprintf(p, buf_len, "%d texture(s) added",
		               r->textures_added);
		p += cnt;
		buf_len -= cnt;
	}

	if (r->textures_overwritten > 0) {
		if (strlen(buf) > 0) {
			cnt = snprintf(p, buf_len, ", ");
			p += cnt;
			buf_len -= cnt;
		}
		snprintf(p, buf_len, "%d texture(s) overwritten",
		         r->textures_overwritten);
	}

	UI_ShowNotice("%s", buf);
}

static void MergePnamesResultNotice(struct texture_bundle_merge_result *r)
{
	char buf[64] = "";
	size_t buf_len = sizeof(buf), cnt;
	char *p = buf;

	if (r->pnames_added == 0) {
		UI_ShowNotice("No new patch names added.");
		return;
	}

	cnt = snprintf(p, buf_len, "%d patch name(s) added", r->pnames_added);
	p += cnt;
	buf_len -= cnt;

	if (r->pnames_present > 0) {
		snprintf(p, buf_len, ", %d already present", r->pnames_present);
	}

	UI_ShowNotice("%s", buf);
}

static void ActionImportTextures(void)
{
	struct texture_bundle_merge_result merge_stats;
	struct texture_bundle b;
	struct texture_bundle *into = TX_DirGetBundle(other_pane->dir);
	struct directory_entry *ent;
	int selected = B_DirectoryPaneSelected(active_pane);
	int insert_pos = B_DirectoryPaneSelected(other_pane) + 1;
	VFILE *in;

	if (selected < 0) {
		return;
	}

	ent = &active_pane->dir->entries[selected];
	in = VFS_OpenByEntry(active_pane->dir, ent);

	ClearConversionErrors();
	if (!TX_BundleParseTextures(&b, in)) {
		UI_MessageBox("Failed to import config from '%s':\n%s",
		              ent->name, GetConversionError());
		return;
	}

	if (!B_CheckReadOnly(other_pane->dir)) {
		return;
	}

	if (TX_BundleConfirmAddPnames(into, &b) &&
	    TX_BundleConfirmTextureOverwrite(into, &b)) {
		TX_BundleMerge(into, insert_pos, &b, &merge_stats);
		VFS_CommitChanges(other_pane->dir, "import from '%s'",
		                  ent->name);

		MergeTexturesResultNotice(&merge_stats);
		VFS_Refresh(other_pane->dir);
		B_SwitchToPane(other_pane);
		// TODO: Highlight new/updated items
	}

	TX_FreeBundle(&b);
}

const struct action import_texture_config = {
    KEY_F(5), 'C', "ImpCfg", "> Import config", ActionImportTextures,
};

static void ActionImportPnames(void)
{
	struct texture_bundle_merge_result merge_stats;
	struct texture_bundle b;
	struct pnames *into = TX_PnamesList(other_pane->dir);
	struct directory_entry *ent;
	int selected = B_DirectoryPaneSelected(active_pane);
	int insert_pos = B_DirectoryPaneSelected(other_pane) + 1;
	VFILE *in;

	if (selected < 0) {
		return;
	}

	ent = &active_pane->dir->entries[selected];
	in = VFS_OpenByEntry(active_pane->dir, ent);

	ClearConversionErrors();
	if (!TX_BundleParsePnames(&b, in)) {
		UI_MessageBox("Failed to import config from '%s':\n%s",
		              ent->name, GetConversionError());
		return;
	}

	if (!B_CheckReadOnly(other_pane->dir)) {
		return;
	}

	TX_MergePnames(into, b.pn, &merge_stats);
	VFS_CommitChanges(other_pane->dir, "import from '%s'", ent->name);

	MergePnamesResultNotice(&merge_stats);
	VFS_Refresh(other_pane->dir);
	B_SwitchToPane(other_pane);
	// TODO: Highlight new/updated items

	TX_FreeBundle(&b);
}

const struct action import_pnames_config = {
    KEY_F(5), 'C', "ImpCfg", "> Import config", ActionImportPnames,
};

static void ActionNewPname(void)
{
	struct pnames *pn = TX_PnamesList(active_pane->dir);
	int idx;
	char *name;

	if (!B_CheckReadOnly(active_pane->dir)) {
		return;
	}

	name = UI_TextInputDialogBox("New pname", "Create", 8,
	                             "Enter new patch name:");
	if (name == NULL) {
		return;
	}

	if (TX_GetPnameIndex(pn, name) >= 0) {
		B_DirectoryPaneSelectByName(active_pane, name);
		UI_MessageBox("'%s' is already in the list.", name);
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
	B_DirectoryPaneSelectEntry(active_pane,
	                           &active_pane->dir->entries[idx]);
	free(name);
}

const struct action new_pname_action = {
    KEY_F(7), 'K', "NewPname", ". New pname", ActionNewPname,
};

static void ActionCopyPnames(void)
{
	struct file_set *tagged = B_DirectoryPaneTagged(active_pane);
	struct file_set copied = EMPTY_FILE_SET;
	struct texture_bundle_merge_result merge_stats;
	struct directory *from_dir = active_pane->dir,
	                 *to_dir = other_pane->dir;
	struct pnames *into = TX_PnamesList(to_dir);
	struct directory_entry *ent;
	int idx;

	if (tagged->num_entries == 0) {
		UI_MessageBox("You have not selected any pnames to copy.");
		return;
	}

	if (!B_CheckReadOnly(to_dir)) {
		return;
	}

	memset(&merge_stats, 0, sizeof(merge_stats));
	idx = 0;
	while ((ent = VFS_IterateSet(from_dir, tagged, &idx)) != NULL) {
		if (TX_GetPnameIndex(into, ent->name) >= 0) {
			++merge_stats.pnames_present;
			continue;
		}
		TX_AppendPname(into, ent->name);
		VFS_AddToSet(&copied, TX_PnameSerialNo(ent->name));
		++merge_stats.pnames_added;
	}

	VFS_Refresh(to_dir);

	MergePnamesResultNotice(&merge_stats);

	if (merge_stats.pnames_added > 0) {
		char buf[32];

		VFS_DescribeSet(to_dir, &copied, buf, sizeof(buf));
		VFS_CommitChanges(to_dir, "copy of %s", buf);
		B_DirectoryPaneSetTagged(other_pane, &copied);
		B_SwitchToPane(other_pane);
	}

	VFS_FreeSet(&copied);
}

const struct action copy_pnames_action = {
    KEY_F(5), 'C', "Copy", "> Copy names", ActionCopyPnames,
};

static void ActionCopyTextures(void)
{
	struct texture_bundle_merge_result merge_stats;
	struct file_set *tagged = B_DirectoryPaneTagged(active_pane);
	struct texture_bundle b;
	struct directory *from_dir = active_pane->dir,
	                 *to_dir = other_pane->dir;
	struct texture_bundle *into_bundle = TX_DirGetBundle(to_dir);
	int insert_pos = B_DirectoryPaneSelected(other_pane) + 1;
	VFILE *marshaled;

	if (tagged->num_entries == 0) {
		UI_MessageBox("You have not selected any textures to copy.");
		return;
	}

	marshaled = FormatConfig(from_dir, tagged);
	assert(marshaled != NULL);

	assert(TX_BundleParseTextures(&b, marshaled));

	if (!B_CheckReadOnly(to_dir)) {
		return;
	}

	if (TX_BundleConfirmAddPnames(into_bundle, &b) &&
	    TX_BundleConfirmTextureOverwrite(into_bundle, &b)) {
		TX_BundleMerge(into_bundle, insert_pos, &b, &merge_stats);
		// TODO: Switch pane; highlight the new/modified textures
	}

	TX_FreeBundle(&b);
	VFS_CommitChanges(to_dir, "copy of %d textures",
	                  merge_stats.textures_added +
	                      merge_stats.textures_overwritten);
	VFS_Refresh(to_dir);

	MergeTexturesResultNotice(&merge_stats);
}

const struct action copy_textures_action = {
    KEY_F(5), 'C', "Copy", "> Copy textures", ActionCopyTextures,
};
