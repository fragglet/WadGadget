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
#include "browser/browser.h"
#include "common.h"
#include "conv/error.h"
#include "stringlib.h"
#include "ui/dialog.h"
#include "ui/title_bar.h"
#include "ui/ui.h"
#include "view.h"

#include "textures/textures.h"
#include "textures/internal.h"

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
	struct directory_revision *old_rev, *orig_rev;
	struct directory *parent;
	struct directory_entry *ent;
	int i;

	parent = TX_DirGetParent(active_pane->dir, &ent);
	orig_rev = parent->curr_revision;

	if (!TX_DirSave(active_pane->dir)) {
		UI_MessageBox("Failed to save directory for editing.");
		return;
	}

	parent = TX_DirGetParent(active_pane->dir, &ent);
	old_rev = parent->curr_revision;
	OpenDirent(parent, ent);

	// Editing may have reset the readonly flag.
	active_pane->dir->readonly = parent->readonly;

	// If the current revision of the WAD has changed since the call to
	// `OpenDirent()`, we know it imported a new version of the lump.
	// So detect this case and import it into the current directory.
	if (!parent->readonly && parent->curr_revision != old_rev
	 && TX_DirReload(active_pane->dir)) {
		VFS_CommitChanges(active_pane->dir, "edit via text editor");
		VFS_ClearSet(&active_pane->tagged);
	}

	// Up to two WAD revisions may have occurred above (via `TX_DirSave`
	// and `OpenDirent`). We can now undo the edit to the WAD; the
	// changes will only takes effect once exiting the current directory.
	for (i = 0; i < 2 && parent->curr_revision != orig_rev; i++) {
		VFS_Undo(parent, 1);
	}
	assert(parent->curr_revision == orig_rev);
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
	vfclose(out_wrapped);

	VFS_Refresh(other_pane->dir);

	B_SwitchToPane(other_pane);
	UI_DirectoryPaneSelectByName(other_pane, filename);

cancel:
	vfclose(formatted);
	free(filename);
	free(filename2);
}

const struct action export_texture_config = {
	KEY_F(5), 'O', "ExpCfg", "> Export config",
	PerformExportConfig,
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
		snprintf(p, buf_len, ", %d already present",
		         r->pnames_present);
	}

	UI_ShowNotice("%s", buf);
}

static void PerformImportConfig(struct directory_pane *active_pane,
                                struct directory_pane *other_pane)
{
	struct texture_bundle_merge_result merge_stats;
	struct texture_bundle b;
	struct texture_bundle *into = TX_DirGetBundle(other_pane->dir);
	struct directory_entry *ent;
	int selected = UI_DirectoryPaneSelected(active_pane);
	int insert_pos = UI_DirectoryPaneSelected(other_pane) + 1;
	VFILE *in;

	if (selected < 0) {
		return;
	}

	if (!CheckReadOnly(other_pane->dir)) {
		return;
	}

	ent = &active_pane->dir->entries[selected];
	in = VFS_OpenByEntry(active_pane->dir, ent);

	ClearConversionErrors();
	if (!TX_DirParseConfig(other_pane->dir, &b, in)) {
		UI_MessageBox("Failed to import config from '%s':\n%s",
		              ent->name, GetConversionError());
		return;
	}

	if (TX_BundleConfirmAddPnames(into, &b)
	 && TX_BundleConfirmTextureOverwrite(into, &b)) {
		TX_BundleMerge(into, insert_pos, &b, &merge_stats);
		VFS_CommitChanges(other_pane->dir, "import from '%s'",
		                  ent->name);

		if (into->txs->num_textures > 0) {
			MergeTexturesResultNotice(&merge_stats);
		} else {
			MergePnamesResultNotice(&merge_stats);
		}
		VFS_Refresh(other_pane->dir);
		B_SwitchToPane(other_pane);
		// TODO: Highlight new/updated items
	}

	TX_FreeBundle(&b);
}

const struct action import_texture_config = {
	KEY_F(5), 'O', "ImpCfg", "> Import config",
	PerformImportConfig,
};

static void PerformNewPname(struct directory_pane *active_pane,
                            struct directory_pane *other_pane)
{
	struct texture_bundle *b = TX_DirGetBundle(active_pane->dir);
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

	if (TX_GetPnameIndex(b->pn, name) >= 0) {
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
	idx = TX_AppendPname(b->pn, name);
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

static void PerformCopyPnames(struct directory_pane *active_pane,
                              struct directory_pane *other_pane)
{
	struct file_set *tagged = UI_DirectoryPaneTagged(active_pane);
	struct file_set copied = EMPTY_FILE_SET;
	struct texture_bundle_merge_result merge_stats;
	struct directory *from_dir = active_pane->dir,
	                 *to_dir = other_pane->dir;
	struct texture_bundle *b = TX_DirGetBundle(to_dir);
	struct directory_entry *ent;
	int idx;

	if (tagged->num_entries == 0) {
		UI_MessageBox("You have not selected any pnames to copy.");
		return;
	}

	if (!CheckReadOnly(to_dir)) {
		return;
	}

	memset(&merge_stats, 0, sizeof(merge_stats));
	idx = 0;
	while ((ent = VFS_IterateSet(from_dir, tagged, &idx)) != NULL) {
		if (TX_GetPnameIndex(b->pn, ent->name) >= 0) {
			++merge_stats.pnames_present;
			continue;
		}
		TX_AppendPname(b->pn, ent->name);
		VFS_AddToSet(&copied, TX_PnameSerialNo(ent->name));
		++merge_stats.pnames_added;
	}

	VFS_Refresh(to_dir);

	MergePnamesResultNotice(&merge_stats);

	if (merge_stats.pnames_added > 0) {
		char buf[32];

		VFS_DescribeSet(to_dir, &copied, buf, sizeof(buf));
		VFS_CommitChanges(to_dir, "copy of %s", buf);
		UI_DirectoryPaneSetTagged(other_pane, &copied);
		B_SwitchToPane(other_pane);
	}

	VFS_FreeSet(&copied);
}

const struct action copy_pnames_action = {
	KEY_F(5), 'O', "Copy", "> Copy names",
	PerformCopyPnames,
};

static void PerformCopyTextures(struct directory_pane *active_pane,
                                struct directory_pane *other_pane)
{
	struct texture_bundle_merge_result merge_stats;
	struct file_set *tagged = UI_DirectoryPaneTagged(active_pane);
	struct texture_bundle b;
	struct directory *from_dir = active_pane->dir,
	                 *to_dir = other_pane->dir;
	struct texture_bundle *into_bundle = TX_DirGetBundle(to_dir);
	int insert_pos = UI_DirectoryPaneSelected(other_pane) + 1;
	VFILE *marshaled;

	if (tagged->num_entries == 0) {
		UI_MessageBox("You have not selected any textures to copy.");
		return;
	}

	if (!CheckReadOnly(to_dir)) {
		return;
	}

	marshaled = TX_DirFormatConfig(from_dir, tagged);
	assert(marshaled != NULL);

	assert(TX_DirParseConfig(to_dir, &b, marshaled));

	if (TX_BundleConfirmAddPnames(into_bundle, &b)
	 && TX_BundleConfirmTextureOverwrite(into_bundle, &b)) {
		TX_BundleMerge(into_bundle, insert_pos, &b, &merge_stats);
	}

	TX_FreeBundle(&b);
	VFS_CommitChanges(to_dir, "copy of %d textures",
	                  merge_stats.textures_added +
	                  merge_stats.textures_overwritten);
	VFS_Refresh(to_dir);

	MergeTexturesResultNotice(&merge_stats);
}

const struct action copy_textures_action = {
	KEY_F(5), 'O', "Copy", "> Copy textures",
	PerformCopyTextures,
};
