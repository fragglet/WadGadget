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

struct genmidi_dir {
	struct directory dir;
	struct genmidi_bank bank;
	struct directory *parent_dir;
};

const struct file_type file_type_genmidi_bank = {"GENMIDI bank"};
const struct file_type file_type_genmidi_voice = {"Voice"};

static void GenmidiDirRefresh(void *dir, struct directory_entry **entries,
                              size_t *num_entries)
{
	int i;

	*entries = checked_calloc(NUM_GENMIDI_INSTRS * 2,
	                          sizeof(struct directory_entry));
	*num_entries = NUM_GENMIDI_INSTRS * 2;

	for (i = 0; i < NUM_GENMIDI_INSTRS * 2; ++i) {
		struct directory_entry *ent = *entries + i;
		ent->type = &file_type_genmidi_voice;
		ent->name = checked_calloc(9, 1);
		snprintf(ent->name, 9, "%d%c", i / 2, "ab"[i % 2]);
		ent->size = 0;
		ent->serial_no = i;
	}
}

static bool GenmidiDirRename(void *dir, struct directory_entry *entry,
                             const char *new_name)
{
	// TODO
	return false;
}

static void GenmidiDirFree(void *_dir)
{
	struct genmidi_dir *dir = _dir;
	VFS_DirectoryUnref(dir->parent_dir);
}

static const struct directory_funcs genmidi_dir_funcs = {
    "Voice",           // singular
    "Voices",          // plural
    true,              // ordered
    GenmidiDirRefresh, // refresh
    NULL,              // open
    NULL,              // open_dir
    NULL,              // remove
    GenmidiDirRename,  // rename
    NULL,              // need_commit
    NULL,              // commit
    NULL,              // swap_entries
    NULL,              // save_snapshot
    NULL,              // restore_snapshot
    GenmidiDirFree,    // free
};

struct directory *GENMIDI_OpenDir(struct directory *parent,
                                  struct directory_entry *ent)
{
	struct genmidi_dir *dir = checked_calloc(1, sizeof(struct genmidi_dir));

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

	dir->parent_dir = parent;
	VFS_DirectoryRef(dir->parent_dir);
	VFS_Refresh(&dir->dir);

	return &dir->dir;
}
