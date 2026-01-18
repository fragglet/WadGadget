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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs/lump_dir.h"
#include "fs/vfile.h"
#include "fs/vfs.h"
#include "genmidi/genmidi.h"
#include "stringlib.h"
#include "ui/title_bar.h"

struct genmidi_dir {
	struct lump_based_dir dir;
	struct genmidi_bank bank;
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

static bool GenmidiDirRemove(void *_dir, struct directory_entry *entry)
{
	VFS_StoreError("Voices cannot be deleted, only replaced.");
	return false;
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

static void GenmidiDirInitEmpty(void *_dir)
{
	struct genmidi_dir *dir = _dir;
	int i;

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		GENMIDI_ClearInstrument(&dir->bank, &dir->bank.instrs[i],
		                        false);
	}

	UI_ShowNotice("Creating a new, empty GENMIDI directory.");
}

static VFILE *GenmidiDirMarshal(void *_dir)
{
	struct genmidi_dir *dir = _dir;
	VFILE *result = vfopenmem(NULL, 0);

	assert(GENMIDI_SaveBank(&dir->bank, result));
	vfseek(result, 0, SEEK_SET);
	return result;
}

static bool GenmidiDirUnmarshal(void *_dir, VFILE *in, int mod_count)
{
	struct genmidi_dir *dir = _dir;
	bool success;
	dir->bank.modified_count = mod_count;
	success = GENMIDI_LoadBank(&dir->bank, in);
	vfclose(in);

	return success;
}

static int GenmidiDirModifiedCount(void *_dir)
{
	struct genmidi_dir *dir = _dir;
	return dir->bank.modified_count;
}

static const struct lump_based_dir_funcs lump_dir_funcs = {
    GenmidiDirInitEmpty,
    GenmidiDirMarshal,
    GenmidiDirUnmarshal,
    GenmidiDirModifiedCount,
};

static const struct directory_funcs genmidi_dir_funcs = {
    "Voice",                    // singular
    "Voices",                   // plural
    true,                       // ordered
    GenmidiDirRefresh,          // refresh
    NULL,                       // open
    VFS_LumpDirOpenDir,         // open_dir
    GenmidiDirRemove,           // remove
    GenmidiDirRename,           // rename
    VFS_LumpDirNeedCommit,      // need_commit
    VFS_LumpDirCommit,          // commit
    NULL,                       // swap_entries
    VFS_LumpDirSaveSnapshot,    // save_snapshot
    VFS_LumpDirRestoreSnapshot, // restore_snapshot
    VFS_LumpDirFree,            // free
};

struct directory *GENMIDI_OpenDir(struct directory *parent,
                                  struct directory_entry *ent)
{
	struct genmidi_dir *dir = checked_calloc(1, sizeof(struct genmidi_dir));

	dir->dir.dir.type = &file_type_genmidi_bank;
	dir->dir.dir.directory_funcs = &genmidi_dir_funcs;

	if (!VFS_LumpDirInit(&dir->dir, &lump_dir_funcs, parent, ent)) {
		free(dir);
		return NULL;
	}

	return &dir->dir.dir;
}
