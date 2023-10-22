
// For want of a better name.
// Things that are in common between WAD files and directory lists.

#ifndef INCLUDED_BLOB_LIST_H
#define INCLUDED_BLOB_LIST_H

#include "vfile.h"

struct blob_tag_list {
	unsigned int *entries;
	size_t num_entries;
};

enum blob_type {
	BLOB_TYPE_FILE,
	BLOB_TYPE_DIR,
	BLOB_TYPE_LUMP,
	BLOB_TYPE_WAD,
};

struct blob_list_entry {
	enum blob_type type;
	char *name;
	ssize_t size;
};

struct blob_list {
	char *path;
	char *parent_dir, *name;
	struct blob_tag_list tags;
	VFILE *(*open_blob)(void *_dir, int lump_index);
	const struct blob_list_entry *(*get_entry)(
		struct blob_list *p, unsigned int i);
	const char *(*get_entry_path)(struct blob_list *l, unsigned int idx);
	void (*free)(struct blob_list *bl);
};

void BL_SetPathFields(void *bl, const char *path);
void BL_FreeList(void *bl);

void BL_AddTag(struct blob_tag_list *l, unsigned int index);
void BL_RemoveTag(struct blob_tag_list *l, unsigned int index);
int BL_IsTagged(struct blob_tag_list *l, unsigned int index);
void BL_ClearTags(struct blob_tag_list *l);
int BL_NumTagged(struct blob_tag_list *l);

// Handle renumbering of indexes after an item is inserted or removed.
void BL_HandleInsert(struct blob_tag_list *l, unsigned int index);
void BL_HandleDelete(struct blob_tag_list *l, unsigned int index);

#endif /* #ifndef INCLUDED_BLOB_LIST_H */

