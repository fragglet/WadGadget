//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "stringlib.h"
#include "fs/vfs.h"

struct directory *VFS_OpenRealDir(const char *path);  // real_dir.c
struct directory *VFS_OpenWadAsDirectory(const char *path);  // wad_dir.c

// Yes, Si units, not binary ones.
#define KB(x) (x * 1000ULL)
#define MB(x) (KB(x) * 1000ULL)
#define GB(x) (MB(x) * 1000ULL)
#define TB(x) (GB(x) * 1000ULL)

struct directory_entry _vfs_parent_directory = {
	FILE_TYPE_DIR, "..", 0, UINT64_MAX,
};

static char last_error[128];
static struct directory *open_dirs = NULL;

static void FreeRevisionChainBackward(struct directory_revision *r)
{
	struct directory_revision *prev;

	while (r != NULL) {
		prev = r->prev;
		free(r->snapshot);
		free(r);
		r = prev;
	}
}

static void FreeRevisionChainForward(struct directory_revision *r)
{
	struct directory_revision *next;

	while (r != NULL) {
		next = r->next;
		free(r->snapshot);
		free(r);
		r = next;
	}
}

char *VFS_EntryPath(struct directory *dir, struct directory_entry *entry)
{
	return StringJoin("/", dir->path, entry->name, NULL);
}

void VFS_FreeEntries(struct directory *d)
{
	int i;

	for (i = 0; i < d->num_entries; i++) {
		free(d->entries[i].name);
	}
	free(d->entries);
	d->entries = NULL;
	d->num_entries = 0;
}

static char *ParentName(const char *path)
{
	char *parent = PathDirName(path);
	char *result = StringJoin("", "Parent (", PathBaseName(parent),
	                          ")", NULL);
	free(parent);
	return result;
}

void VFS_InitDirectory(struct directory *d, const char *path)
{
	// By default we assume no snapshot/undo capability. The directory
	// implementation must call VFS_SaveRevision() in its init function
	// if it is supported.
	d->curr_revision = NULL;
	d->path = PathSanitize(path);
	d->parent_name = ParentName(d->path);
	d->refcount = 1;
	d->entries = NULL;
	d->num_entries = 0;
}

static struct directory *FindOpenDir(const char *path)
{
	struct directory *d = open_dirs;

	while (d != NULL) {
		if (!strcmp(d->path, path)) {
			return d;
		}
		d = d->next;
	}

	return NULL;
}

static void HookOpenDir(struct directory *d)
{
	d->next = open_dirs;
	open_dirs = d;
}

static void UnhookOpenDir(struct directory *d)
{
	struct directory **nextptr = &open_dirs;

	while (*nextptr != NULL) {
		if (*nextptr == d) {
			*nextptr = d->next;
			break;
		}
		nextptr = &(*nextptr)->next;
	}
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

struct directory *VFS_OpenDir(const char *path)
{
	struct directory *d;
	char *sanitized_path = PathSanitize(path);

	// If the directory is already open, just return the existing
	// instance. This means that it's never possible for the two
	// panes to go out of sync, eg. if a new file is created.
	d = FindOpenDir(sanitized_path);
	if (d != NULL) {
		VFS_DirectoryRef(d);
		free(sanitized_path);
		return d;
	}

	// TODO: This is kind of a hack.
	if (HasWadExtension(sanitized_path)) {
		d = VFS_OpenWadAsDirectory(sanitized_path);
	} else {
		d = VFS_OpenRealDir(sanitized_path);
	}

	free(sanitized_path);

	if (d != NULL) {
		HookOpenDir(d);
	}
	return d;
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

struct directory *VFS_OpenDirByEntry(struct directory *dir,
                                     struct directory_entry *entry)
{
	return dir->directory_funcs->open_dir(dir, entry);
}

struct directory_entry *VFS_EntryBySerial(struct directory *dir,
                                          uint64_t serial_no)
{
	int i;

	for (i = 0; i < dir->num_entries; i++) {
		if (dir->entries[i].serial_no == serial_no) {
			return &dir->entries[i];
		}
	}

	return NULL;
}

struct directory_entry *VFS_EntryByName(struct directory *dir,
                                        const char *name)
{
	int i;

	for (i = 0; i < dir->num_entries; i++) {
		if (!strcmp(dir->entries[i].name, name)) {
			return &dir->entries[i];
		}
	}

	return NULL;
}

// VFS_SaveRevision takes a new snapshot and creates a new directory_revision
// containing it. It does not commit any changes and should not modify the
// underlying directory, but the snapshotted data can be used to restore the
// directory back to its old state later. Compare with VFS_CommitChanges which
// does change the underlying directory by writing out any pending changes.
// VFS_SaveRevision is called by VFS_CommitChanges after committing new
// changes, but also on initialize to create the first revision of a directory.
struct directory_revision *VFS_SaveRevision(struct directory *dir)
{
	struct directory_revision *result;
	VFILE *out;

	if (dir->directory_funcs->save_snapshot == NULL) {
		return NULL;
	}

	out = dir->directory_funcs->save_snapshot(dir);
	if (out == NULL) {
		return NULL;
	}

	result = checked_calloc(1, sizeof(struct directory_revision));
	result->snapshot = vfreadall(out, &result->snapshot_len);

	result->prev = dir->curr_revision;
	if (dir->curr_revision != NULL) {
		// Ditch all current redo history.
		FreeRevisionChainForward(dir->curr_revision->next);
		dir->curr_revision->next = result;
	}
	dir->curr_revision = result;
	return result;
}

void VFS_CommitChanges(struct directory *dir, const char *msg, ...)
{
	struct directory_revision *rev;
	va_list args;

	if (dir->readonly
	 || dir->directory_funcs->commit == NULL
	 || dir->directory_funcs->need_commit == NULL
	 || !dir->directory_funcs->need_commit(dir)) {
		return;
	}

	dir->directory_funcs->commit(dir);

	if (dir->curr_revision != NULL) {
		va_start(args, msg);
		rev = VFS_SaveRevision(dir);
		vsnprintf(rev->descr, VFS_REVISION_DESCR_LEN, msg, args);
		va_end(args);
	}
}

// Reload the list of entries for the given directory, returning the index
// of the first entry to change (or -1 for no change)
int VFS_Refresh(struct directory *dir)
{
	struct directory_entry *entries = NULL;
	size_t num_entries = 0;
	int i, result;

	dir->directory_funcs->refresh(dir, &entries, &num_entries);

	// Find the first entry to have changed between the old and new.
	result = -1;
	for (i = 0; i < num_entries; i++) {
		if (i >= dir->num_entries
		 || strcmp(entries[i].name, dir->entries[i].name) != 0
		 || entries[i].size != dir->entries[i].size) {
			result = i;
			break;
		}
	}

	VFS_FreeEntries(dir);
	dir->entries = entries;
	dir->num_entries = num_entries;

	return result;
}

bool VFS_Remove(struct directory *dir, struct directory_entry *entry)
{
	unsigned int index = entry - dir->entries;

	if (dir->readonly) {
		VFS_StoreError("VFS directory is read only.");
		return false;
	}

	VFS_StoreError("");
	if (!dir->directory_funcs->remove(dir, entry)) {
		return false;
	}

	memmove(&dir->entries[index], &dir->entries[index + 1],
	        (dir->num_entries - index - 1)
	          * sizeof(struct directory_entry));
	--dir->num_entries;

	return true;
}

bool VFS_Rename(struct directory *dir, struct directory_entry *entry,
                const char *new_name)
{
	if (dir->readonly) {
		VFS_StoreError("VFS directory is read only.");
		return false;
	}

	VFS_StoreError("");
	return dir->directory_funcs->rename(dir, entry, new_name);
}

void VFS_DirectoryRef(struct directory *dir)
{
	++dir->refcount;
}

void VFS_DirectoryUnref(struct directory *dir)
{
	--dir->refcount;

	if (dir->refcount > 0) {
		return;
	}

	UnhookOpenDir(dir);

	if (dir->directory_funcs->free != NULL) {
		dir->directory_funcs->free(dir);
	}
	if (dir->curr_revision != NULL) {
		FreeRevisionChainBackward(dir->curr_revision->prev);
		FreeRevisionChainForward(dir->curr_revision);
	}
	VFS_FreeEntries(dir);
	free(dir->parent_name);
	free(dir->path);
	free(dir);
}

void VFS_DescribeSize(const struct directory_entry *ent, char buf[10],
                      bool shorter)
{
	int64_t adj_len = ent->size * (shorter ? 100 : 1);

	if (adj_len < 0) {
		strncpy(buf, "", 10);
	} else if (adj_len < KB(100)) {  // up to 99999
		snprintf(buf, 10, "%d", (int) ent->size);
	} else if (adj_len < MB(10)) {  // up to 9999K
		snprintf(buf, 10, "%dK", (short) (ent->size / KB(1)));
	} else if (adj_len < GB(10)) {  // up to 9999M
		snprintf(buf, 10, "%dM", (short) (ent->size / MB(1)));
	} else if (adj_len < TB(10)) {  // up to 9999G
		snprintf(buf, 10, "%dG", (short) (ent->size / GB(1)));
	} else {
		snprintf(buf, 10, "big!");
	}
}

bool VFS_SwapEntries(struct directory *dir, unsigned int x, unsigned int y)
{
	struct directory_entry tmp;
	if (dir->readonly || dir->directory_funcs->swap_entries == NULL) {
		return false;
	}
	dir->directory_funcs->swap_entries(dir, x, y);
	tmp = dir->entries[x];
	dir->entries[x] = dir->entries[y];
	dir->entries[y] = tmp;
	return true;
}

int VFS_CanUndo(struct directory *dir)
{
	struct directory_revision *r = dir->curr_revision;
	int result = 0;

	if (dir->readonly
	 || dir->curr_revision == NULL
	 || dir->directory_funcs->restore_snapshot == NULL) {
		return 0;
	}

	while (r->prev != NULL) {
		r = r->prev;
		++result;
	}

	return result;
}

void VFS_Undo(struct directory *dir, unsigned int levels)
{
	struct directory_revision *r = dir->curr_revision;
	VFILE *in;
	int i;

	assert(!dir->readonly);
	assert(dir->directory_funcs->restore_snapshot != NULL);

	for (i = 0; i < levels; i++) {
		assert(r->prev != NULL);
		r = r->prev;
	}
	dir->curr_revision = r;

	in = vfopenmem(r->snapshot, r->snapshot_len);
	dir->directory_funcs->restore_snapshot(dir, in);
}

int VFS_CanRedo(struct directory *dir)
{
	struct directory_revision *r = dir->curr_revision;
	int result = 0;

	if (dir->readonly
	 || dir->curr_revision == NULL
	 || dir->directory_funcs->restore_snapshot == NULL) {
		return 0;
	}

	while (r->next != NULL) {
		r = r->next;
		++result;
	}

	return result;
}

void VFS_Redo(struct directory *dir, unsigned int levels)
{
	struct directory_revision *r = dir->curr_revision;
	VFILE *in;
	int i;

	assert(!dir->readonly);
	assert(dir->directory_funcs->restore_snapshot != NULL);

	for (i = 0; i < levels; i++) {
		assert(r->next != NULL);
		r = r->next;
	}
	dir->curr_revision = r;

	in = vfopenmem(r->snapshot, r->snapshot_len);
	dir->directory_funcs->restore_snapshot(dir, in);
}

const char *VFS_LastCommitMessage(struct directory *dir)
{
	if (dir->curr_revision != NULL) {
		return dir->curr_revision->descr;
	}
	return "No history";
}

void VFS_ClearHistory(struct directory *dir)
{
	if (dir->curr_revision == NULL) {
		return;
	}

	FreeRevisionChainBackward(dir->curr_revision->prev);
	FreeRevisionChainForward(dir->curr_revision);
	dir->curr_revision = NULL;

	dir->curr_revision = VFS_SaveRevision(dir);
	snprintf(dir->curr_revision->descr, VFS_REVISION_DESCR_LEN,
	         "First revision");
}

void VFS_StoreError(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(last_error, sizeof(last_error), fmt, args);
	va_end(args);
}

const char *VFS_LastError(void)
{
	return last_error;
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
