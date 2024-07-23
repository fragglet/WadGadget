//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef INCLUDED_VFS_H
#define INCLUDED_VFS_H

#include "fs/vfile.h"
#include "fs/wad_file.h"

#define VFS_REVISION_DESCR_LEN  40
#define VFS_PARENT_DIRECTORY (&_vfs_parent_directory)

enum file_type {
	FILE_TYPE_DIR,
	FILE_TYPE_WAD,
	FILE_TYPE_TEXTURE_LIST,
	FILE_TYPE_PNAMES_LIST,
	FILE_TYPE_PALETTES,
	NUM_DIR_FILE_TYPES,
	FILE_TYPE_FILE,
	FILE_TYPE_LUMP,
	FILE_TYPE_TEXTURE,
	FILE_TYPE_PNAME,
	FILE_TYPE_PALETTE,
};

#define EMPTY_FILE_SET {NULL, 0}

struct file_set {
	uint64_t *entries;
	size_t num_entries;
};

struct directory_entry {
	enum file_type type;
	char *name;
	int64_t size;
	uint64_t serial_no;
};

struct directory_funcs {
	void (*refresh)(void *dir, struct directory_entry **entries,
	                size_t *num_entries);
	VFILE *(*open)(void *dir, struct directory_entry *entry);
	struct directory *(*open_dir)(void *dir,
	                              struct directory_entry *entry);
	bool (*remove)(void *dir, struct directory_entry *entry);
	bool (*rename)(void *dir, struct directory_entry *entry,
	               const char *new_name);
	bool (*need_commit)(void *dir);
	void (*commit)(void *dir);
	void (*describe_entries)(char *buf, size_t buf_len, int cnt);
	void (*swap_entries)(void *dir, unsigned int x, unsigned int y);
	VFILE *(*save_snapshot)(void *dir);
	void (*restore_snapshot)(void *dir, VFILE *in);
	// TODO: insert
	void (*free)(void *dir);
};

struct directory_revision {
	char descr[VFS_REVISION_DESCR_LEN];
	void *snapshot;
	size_t snapshot_len;
	struct directory_revision *prev, *next;
};


struct directory {
	enum file_type type;
	const struct directory_funcs *directory_funcs;
	char *parent_name;
	bool readonly;
	char *path;
	int refcount;
	struct directory_entry *entries;
	size_t num_entries;
	struct directory_revision *curr_revision;
	struct directory *next;
};

struct directory *VFS_OpenDir(const char *path);
struct directory *VFS_OpenDirByEntry(struct directory *dir,
                                     struct directory_entry *entry);

VFILE *VFS_Open(const char *path);
VFILE *VFS_OpenByEntry(struct directory *dir, struct directory_entry *entry);
bool VFS_Remove(struct directory *dir, struct directory_entry *entry);
bool VFS_Rename(struct directory *dir, struct directory_entry *entry,
                const char *new_name);
void VFS_CommitChanges(struct directory *dir, const char *msg, ...);
int VFS_Refresh(struct directory *dir);
struct wad_file *VFS_WadFile(struct directory *dir);
char *VFS_EntryPath(struct directory *dir, struct directory_entry *entry);
struct directory_entry *VFS_EntryBySerial(struct directory *p,
                                          uint64_t serial_no);
struct directory_entry *VFS_EntryByName(struct directory *dir,
                                        const char *name);
struct directory_entry *VFS_IterateSet(struct directory *dir,
                                       struct file_set *set, int *idx);
void VFS_DescribeSet(struct directory *dir, struct file_set *set,
                     char *buf, size_t buf_len);
void VFS_DescribeSize(const struct directory_entry *ent, char buf[10],
                      bool shorter);
bool VFS_SwapEntries(struct directory *dir, unsigned int x, unsigned int y);

void VFS_DirectoryRef(struct directory *dir);
void VFS_DirectoryUnref(struct directory *dir);
#define VFS_CloseDir VFS_DirectoryUnref

void VFS_ClearSet(struct file_set *l);
void VFS_AddToSet(struct file_set *l, uint64_t serial_no);
void VFS_RemoveFromSet(struct file_set *l, uint64_t serial_no);
struct directory_entry *VFS_AddGlobToSet(
	struct directory *dir, struct file_set *l, const char *glob);
bool VFS_SetHas(struct file_set *l, uint64_t serial_no);
void VFS_CopySet(struct file_set *to, struct file_set *from);
void VFS_FreeSet(struct file_set *set);

int VFS_CanUndo(struct directory *dir);
void VFS_Undo(struct directory *dir, unsigned int levels);
int VFS_CanRedo(struct directory *dir);
void VFS_Redo(struct directory *dir, unsigned int levels);
#define VFS_Rollback(d) VFS_Undo(d, 0)
const char *VFS_LastCommitMessage(struct directory *dir);
void VFS_ClearHistory(struct directory *dir);

// For use by implementations of struct directory.
void VFS_InitDirectory(struct directory *d, const char *path);
struct directory_revision *VFS_SaveRevision(struct directory *d);
void VFS_FreeEntries(struct directory *d);

void VFS_StoreError(const char *fmt, ...);
const char *VFS_LastError(void);

extern struct directory_entry _vfs_parent_directory;

#endif /* #ifndef INCLUDED_VFS_H */
