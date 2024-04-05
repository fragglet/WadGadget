#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "strings.h"
#include "vfs.h"
#include "wad_file.h"

struct wad_directory {
	struct directory dir;
	struct wad_file *wad_file;
};

char *VFS_EntryPath(struct directory *dir, struct directory_entry *entry)
{
	return StringJoin("/", dir->path, entry->name, NULL);
}

static void FreeEntries(struct directory *d)
{
	int i;

	for (i = 0; i < d->num_entries; i++) {
		free(d->entries[i].name);
	}
	free(d->entries);
	d->entries = NULL;
	d->num_entries = 0;
}

static int HasWadExtension(const char *name)
{
	const char *extn;
	if (strlen(name) < 4) {
		return 0;
	}
	extn = name + strlen(name) - 4;
	return !strcasecmp(extn, ".wad");
}

static int OrderByName(const void *x, const void *y)
{
	const struct directory_entry *dx = x, *dy = y;
	// Directories get listed before files.
	int cmp = (dy->type == FILE_TYPE_DIR)
	        - (dx->type == FILE_TYPE_DIR);
	if (cmp != 0) {
		return cmp;
	}
	return strcasecmp(dx->name, dy->name);
}

static void RealDirRefresh(void *_d)
{
	struct directory *d = _d;
	DIR *dir;

	FreeEntries(d);

	dir = opendir(d->path);
	assert(dir != NULL);  // TODO

	for (;;) {
		struct dirent *dirent = readdir(dir);
		struct directory_entry *ent;
		char *path;
		if (dirent == NULL) {
			break;
		}
		if (dirent->d_name[0] == '.') {
			continue;
		}
		path = checked_strdup(dirent->d_name);

		d->entries = checked_realloc(d->entries,
			sizeof(struct directory_entry) * (d->num_entries + 1));
		ent = &d->entries[d->num_entries];
		ent->name = path;
		ent->type = dirent->d_type == DT_DIR ? FILE_TYPE_DIR :
		            HasWadExtension(ent->name) ? FILE_TYPE_WAD :
		            FILE_TYPE_FILE;
		ent->size = -1;
		ent->serial_no = dirent->d_ino;
		++d->num_entries;
	}

	closedir(dir);

	qsort(d->entries, d->num_entries, sizeof(struct directory_entry),
	      OrderByName);
}

static VFILE *RealDirOpen(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	FILE *fs;

	fs = fopen(filename, "r+");
	free(filename);

	if (fs == NULL) {
		return NULL;
	}

	return vfwrapfile(fs);
}

static void RealDirRemove(void *_dir, struct directory_entry *entry)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	remove(filename);
	free(filename);
}

static void RealDirRename(void *_dir, struct directory_entry *entry,
                          const char *new_name)
{
	struct directory *dir = _dir;
	char *filename = VFS_EntryPath(dir, entry);
	char *full_new_name = StringJoin("/", dir->path, new_name, NULL);
	rename(filename, full_new_name);
	free(filename);
	free(full_new_name);
}

static const struct directory_funcs realdir_funcs = {
	RealDirRefresh,
	RealDirOpen,
	RealDirRemove,
	RealDirRename,
	NULL,
};

static void WadDirectoryRefresh(void *_dir)
{
	struct wad_directory *dir = _dir;
	struct wad_file_entry *waddir = W_GetDirectory(dir->wad_file);
	unsigned int i, num_lumps = W_NumLumps(dir->wad_file);
	struct directory_entry *ent;

	FreeEntries(&dir->dir);

	dir->dir.num_entries = num_lumps;
	dir->dir.entries = checked_calloc(
		num_lumps, sizeof(struct directory_entry));

	for (i = 0; i < num_lumps; i++) {
		ent = &dir->dir.entries[i];
		ent->type = FILE_TYPE_LUMP;
		ent->name = checked_calloc(9, 1);
		memcpy(ent->name, waddir[i].name, 8);
		ent->name[8] = '\0';
		ent->size = waddir[i].size;
		ent->serial_no = waddir[i].serial_no;
	}
}

static VFILE *WadDirOpen(void *_dir, struct directory_entry *entry)
{
	struct wad_directory *dir = _dir;
	// TODO: We shoud ideally do something that will more reliably
	// map back to the WAD file lump number after inserting and
	// deleting lumps:
	unsigned int lump_index = entry - dir->dir.entries;

	return W_OpenLump(dir->wad_file, lump_index);
}

static void WadDirRemove(void *_dir, struct directory_entry *entry)
{
	struct wad_directory *dir = _dir;
	W_DeleteEntry(dir->wad_file, entry - dir->dir.entries);
}

static void WadDirRename(void *_dir, struct directory_entry *entry,
                         const char *new_name)
{
	struct wad_directory *dir = _dir;
	// TODO: Check new name is valid?
	W_SetLumpName(dir->wad_file, entry - dir->dir.entries, new_name);
}

static void WadDirFree(void *_dir)
{
	struct wad_directory *dir = _dir;
	W_CloseFile(dir->wad_file);
}

static const struct directory_funcs waddir_funcs = {
	WadDirectoryRefresh,
	WadDirOpen,
	WadDirRemove,
	WadDirRename,
	WadDirFree,
};

static void InitDirectory(struct directory *d, const char *path)
{
	d->path = PathSanitize(path);
	d->refcount = 1;
	d->entries = NULL;
	d->num_entries = 0;
}

static struct directory *OpenWadAsDirectory(const char *path)
{
	struct wad_directory *d =
		checked_calloc(1, sizeof(struct wad_directory));

	d->dir.directory_funcs = &waddir_funcs;
	InitDirectory(&d->dir, path);
	d->dir.type = FILE_TYPE_WAD;
	d->wad_file = W_OpenFile(path);
	assert(d->wad_file != NULL);
	WadDirectoryRefresh(d);

	return &d->dir;
}

struct directory *VFS_OpenDir(const char *path)
{
	struct directory *d;

	// TODO: This is kind of a hack.
	if (HasWadExtension(path)) {
		return OpenWadAsDirectory(path);
	}

	d = checked_calloc(1, sizeof(struct directory));

	d->directory_funcs = &realdir_funcs;
	InitDirectory(d, path);
	d->type = FILE_TYPE_DIR;
	RealDirRefresh(d);

	return d;
}

struct directory *VFS_OpenDirByEntry(struct directory *dir,
                                     struct directory_entry *entry)
{
	char *path = VFS_EntryPath(dir, entry);
	struct directory *result = NULL;

	switch (entry->type) {
	case FILE_TYPE_DIR:
		result = VFS_OpenDir(path);
		break;

	case FILE_TYPE_WAD:
		result = OpenWadAsDirectory(path);
		break;

	default:
		break;
	}

	free(path);
	return result;
}

VFILE *VFS_Open(const char *path)
{
	FILE *fs;

	fs = fopen(path, "r+");
	if (fs == NULL) {
		return NULL;
	}

	return vfwrapfile(fs);
}

VFILE *VFS_OpenByEntry(struct directory *dir, struct directory_entry *entry)
{
	return dir->directory_funcs->open(dir, entry);
}

void VFS_Refresh(struct directory *dir)
{
	dir->directory_funcs->refresh(dir);
}

void VFS_Remove(struct directory *dir, struct directory_entry *entry)
{
	dir->directory_funcs->remove(dir, entry);
	VFS_Refresh(dir);
}

void VFS_Rename(struct directory *dir, struct directory_entry *entry,
                const char *new_name)
{
	dir->directory_funcs->rename(dir, entry, new_name);
	VFS_Refresh(dir);
}

void VFS_DirectoryRef(struct directory *dir)
{
	++dir->refcount;
}

void VFS_DirectoryUnref(struct directory *dir)
{
	--dir->refcount;

	if (dir->refcount == 0) {
		if (dir->directory_funcs->free != NULL) {
			dir->directory_funcs->free(dir);
		}
		FreeEntries(dir);
		free(dir->path);
		free(dir);
	}
}

struct wad_file *VFS_WadFile(struct directory *dir)
{
	struct wad_directory *wdir;

	if (dir->type != FILE_TYPE_WAD) {
		return NULL;
	}

	wdir = (struct wad_directory *) dir;
	return wdir->wad_file;
}

#ifdef TEST
int main(int argc, char *argv[])
{
	struct directory *dir;
	int i;

	dir = VFS_OpenDir(argv[1]);

	for (i = 0; i < dir->num_entries; i++) {
		printf("%4d%8d %s\n", dir->entries[i].type,
		       dir->entries[i].size, dir->entries[i].name);
	}

	return 0;
}
#endif
