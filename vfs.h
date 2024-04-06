
#ifndef INCLUDED_VFS_H
#define INCLUDED_VFS_H

#include "vfile.h"
#include "wad_file.h"

enum file_type {
	FILE_TYPE_FILE,
	FILE_TYPE_DIR,
	FILE_TYPE_LUMP,
	FILE_TYPE_WAD,
};

#define EMPTY_FILE_SET {NULL, 0}

struct file_set {
	uint64_t *entries;
	size_t num_entries;
};

struct directory_entry {
	enum file_type type;
	char *name;
	ssize_t size;
	uint64_t serial_no;
};

struct directory_funcs {
	void (*refresh)(void *dir);
	VFILE *(*open)(void *dir, struct directory_entry *entry);
	void (*remove)(void *dir, struct directory_entry *entry);
	void (*rename)(void *dir, struct directory_entry *entry,
	               const char *new_name);
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

VFILE *VFS_Open(const char *path);
VFILE *VFS_OpenByEntry(struct directory *dir, struct directory_entry *entry);
void VFS_Remove(struct directory *dir, struct directory_entry *entry);
void VFS_Rename(struct directory *dir, struct directory_entry *entry,
                const char *new_name);
void VFS_Refresh(struct directory *dir);
struct wad_file *VFS_WadFile(struct directory *dir);
char *VFS_EntryPath(struct directory *dir, struct directory_entry *entry);
struct directory_entry *VFS_EntryBySerial(struct directory *p,
                                          uint64_t serial_no);
struct directory_entry *VFS_EntryByName(struct directory *dir,
                                        const char *name);

void VFS_DirectoryRef(struct directory *dir);
void VFS_DirectoryUnref(struct directory *dir);

void VFS_ClearSet(struct file_set *l);
void VFS_AddToSet(struct file_set *l, unsigned int serial_no);
void VFS_RemoveFromSet(struct file_set *l, unsigned int serial_no);
int VFS_SetHas(struct file_set *l, unsigned int serial_no);
void VFS_CopySet(struct file_set *to, struct file_set *from);
void VFS_FreeSet(struct file_set *set);

#define VFS_CloseDir VFS_DirectoryUnref

#endif /* #ifndef INCLUDED_VFS_H */
