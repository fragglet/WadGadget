//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "genmidi/genmidi.h"
#include "stringlib.h"
#include "ui/dialog.h"
#include "ui/title_bar.h"

struct genmidi_dir {
	struct directory dir;
	struct genmidi_bank bank;
	struct directory *parent_dir;
	uint64_t lump_serial;
	int last_commit;
};

const struct file_type file_type_genmidi_bank = {"GENMIDI bank"};
const struct file_type file_type_genmidi_voice = {"Voice"};

struct genmidi_bank *GENMIDI_DirGetBank(struct directory *_dir)
{
	struct genmidi_dir *dir = (struct genmidi_dir *) _dir;
	return &dir->bank;
}

static const char *InstrumentNumber(int index)
{
	static char buf[16];
	const char *prefix = "";
	int instr_num;

	if (index >= 256) {
		instr_num = (index / 2) - 128 + 35;
		prefix = "p";
	} else {
		instr_num = (index / 2) + 1;
	}
	snprintf(buf, sizeof(buf), "%s%d%c", prefix, instr_num,
	         "ab"[index % 2]);
	return buf;
}

static void GenmidiDirRefresh(void *_dir, struct directory_entry **entries,
                              size_t *num_entries)
{
	struct genmidi_dir *dir = _dir;
	const char *name;
	char buf[64];
	int i;

	*entries = checked_calloc(NUM_GENMIDI_INSTRS * 2,
	                          sizeof(struct directory_entry));
	*num_entries = NUM_GENMIDI_INSTRS * 2;

	for (i = 0; i < NUM_GENMIDI_INSTRS * 2; ++i) {
		struct genmidi_instrument *instr;
		struct directory_entry *ent = *entries + i;

		instr = &dir->bank.instrs[i / 2];
		ent->type = &file_type_genmidi_voice;
		name = instr->name;
		if ((i % 2) == 1 &&
		    (instr->hdr.flags & GENMIDI_FLAG_2VOICE) == 0) {
			name = "(unused)";
		}
		snprintf(buf, sizeof(buf), "%s %s", InstrumentNumber(i), name);
		ent->name = checked_strdup(buf);
		ent->size = 0;
		ent->serial_no = i;
	}
}

static struct directory *GenmidiOpenDir(void *_dir,
                                        struct directory_entry *entry)
{
	struct genmidi_dir *dir = _dir;

	if (entry == VFS_PARENT_DIRECTORY) {
		VFS_DirectoryRef(dir->parent_dir);
		return dir->parent_dir;
	}

	return NULL;
}

static bool GenmidiDirRename(void *_dir, struct directory_entry *entry,
                             const char *new_name)
{
	struct genmidi_dir *dir = _dir;
	const char *prefix = InstrumentNumber(entry->serial_no);

	if ((entry->serial_no % 2) == 1) {
		VFS_StoreError("You can't rename the second voice.");
		return false;
	}

	if (StringHasPrefix(new_name, prefix) &&
	    new_name[strlen(prefix)] == ' ') {
		new_name += strlen(prefix) + 1;
	}

	if (strlen(new_name) > GENMIDI_MAX_INSTR_LEN - 1) {
		return false;
	}

	snprintf(dir->bank.instrs[entry->serial_no / 2].name,
	         GENMIDI_MAX_INSTR_LEN, "%s", new_name);
	++dir->bank.modified_count;
	return true;
}

static void SaveBank(struct genmidi_dir *dir)
{
	struct wad_file *wf;
	struct directory_entry *ent;
	VFILE *out;

	if (dir->last_commit == 0) {
		return;
	}

	ent = VFS_EntryBySerial(dir->parent_dir, dir->lump_serial);
	if (ent == NULL) {
		return;
	}

	wf = VFS_WadFile(dir->parent_dir);
	out = W_OpenLumpRewrite(wf, ent - dir->parent_dir->entries);
	assert(GENMIDI_SaveBank(&dir->bank, out));
	vfclose(out);

	VFS_CommitChanges(dir->parent_dir, "update of GENMIDI bank");
	UI_ShowNotice("%s lump updated.", ent->name);
}

static void GenmidiDirFree(void *_dir)
{
	struct genmidi_dir *dir = _dir;

	SaveBank(dir);
	VFS_DirectoryUnref(dir->parent_dir);
}

static bool GenmidiDirNeedCommit(void *_dir)
{
	struct genmidi_dir *dir = _dir;

	return dir->bank.modified_count > dir->last_commit;
}

static void GenmidiDirCommit(void *_dir)
{
	struct genmidi_dir *dir = _dir;

	dir->last_commit = dir->bank.modified_count;
}

static VFILE *GenmidiDirSaveSnapshot(void *_dir)
{
	struct genmidi_dir *dir = _dir;
	VFILE *result = vfopenmem(NULL, 0);

	assert(vfwrite(&dir->bank, sizeof(struct genmidi_bank), 1, result) ==
	       1);
	vfseek(result, 0, SEEK_SET);
	return result;
}

static void GenmidiDirRestoreSnapshot(void *_dir, VFILE *in)
{
	struct genmidi_dir *dir = _dir;
	assert(vfread(&dir->bank, sizeof(struct genmidi_bank), 1, in) == 1);
	vfclose(in);
	dir->last_commit = dir->bank.modified_count;
}

static const struct directory_funcs genmidi_dir_funcs = {
    "Voice",                   // singular
    "Voices",                  // plural
    true,                      // ordered
    GenmidiDirRefresh,         // refresh
    NULL,                      // open
    GenmidiOpenDir,            // open_dir
    NULL,                      // remove
    GenmidiDirRename,          // rename
    GenmidiDirNeedCommit,      // need_commit
    GenmidiDirCommit,          // commit
    NULL,                      // swap_entries
    GenmidiDirSaveSnapshot,    // save_snapshot
    GenmidiDirRestoreSnapshot, // restore_snapshot
    GenmidiDirFree,            // free
};

struct directory *GENMIDI_OpenDir(struct directory *parent,
                                  struct directory_entry *ent)
{
	struct genmidi_dir *dir = checked_calloc(1, sizeof(struct genmidi_dir));
	struct directory_revision *rev;
	VFILE *lump;
	bool loaded;

	lump = VFS_OpenByEntry(parent, ent);
	loaded = GENMIDI_LoadBank(&dir->bank, lump);
	vfclose(lump);
	if (!loaded) {
		free(dir);
		return NULL;
	}

	dir->dir.directory_funcs = &genmidi_dir_funcs;
	dir->dir.type = &file_type_genmidi_bank;
	dir->dir.path =
	    StringJoin(DIR_SEPARATOR_S, parent->path, ent->name, NULL);
	dir->dir.refcount = 1;
	dir->dir.entries = NULL;
	dir->dir.num_entries = 0;
	dir->dir.readonly = parent->readonly;
	dir->dir.parent_name =
	    StringJoin("", "Back to ", PathBaseName(parent->path), NULL);

	dir->last_commit = 0;
	dir->parent_dir = parent;
	dir->lump_serial = ent->serial_no;
	VFS_DirectoryRef(dir->parent_dir);
	VFS_Refresh(&dir->dir);

	rev = VFS_SaveRevision(&dir->dir);
	snprintf(rev->descr, VFS_REVISION_DESCR_LEN, "Initial version");

	return &dir->dir;
}
