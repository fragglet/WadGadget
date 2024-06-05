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

#define VFS_PARENT_DIRECTORY (&_vfs_parent_directory)

enum file_type {
	FILE_TYPE_DIR,
	FILE_TYPE_WAD,
	FILE_TYPE_TEXTURE_LIST,
	NUM_DIR_FILE_TYPES,
	FILE_TYPE_FILE,
	FILE_TYPE_LUMP,
	FILE_TYPE_TEXTURE,
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
	void (*refresh)(void *dir);
	VFILE *(*open)(void *dir, struct directory_entry *entry);
	struct directory *(*open_dir)(void *dir,
	                              struct directory_entry *entry);
	void (*remove)(void *dir, struct directory_entry *entry);
	void (*rename)(void *dir, struct directory_entry *entry,
	               const char *new_name);
	void (*commit)(void *dir);
	void (*describe_entries)(char *buf, size_t buf_len, int cnt);
	// TODO: insert
	void (*free)(void *dir);
};

struct directory {
	enum file_type type;
	const struct directory_funcs *directory_funcs;
	char *path;
	int refcount;
	struct directory_entry *entries;
	size_t num_entries;
};

struct directory *VFS_OpenDir(const char *path);
struct directory *VFS_OpenDirByEntry(struct directory *dir,
                                     struct directory_entry *entry);
struct directory *VFS_OpenWadAsDirectory(const char *path);

VFILE *VFS_Open(const char *path);
VFILE *VFS_OpenByEntry(struct directory *dir, struct directory_entry *entry);
void VFS_Remove(struct directory *dir, struct directory_entry *entry);
void VFS_Rename(struct directory *dir, struct directory_entry *entry,
                const char *new_name);
void VFS_CommitChanges(struct directory *dir);
void VFS_Refresh(struct directory *dir);
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

void VFS_DirectoryRef(struct directory *dir);
void VFS_DirectoryUnref(struct directory *dir);
#define VFS_CloseDir VFS_DirectoryUnref

void VFS_ClearSet(struct file_set *l);
void VFS_AddToSet(struct file_set *l, unsigned int serial_no);
void VFS_RemoveFromSet(struct file_set *l, unsigned int serial_no);
struct directory_entry *VFS_AddGlobToSet(
	struct directory *dir, struct file_set *l, const char *glob);
int VFS_SetHas(struct file_set *l, unsigned int serial_no);
void VFS_CopySet(struct file_set *to, struct file_set *from);
void VFS_FreeSet(struct file_set *set);

// For use by implementations of struct directory.
void VFS_InitDirectory(struct directory *d, const char *path);
void VFS_FreeEntries(struct directory *d);

extern struct directory_entry _vfs_parent_directory;

#endif /* #ifndef INCLUDED_VFS_H */
