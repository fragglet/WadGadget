
// For want of a better name.
// Things that are in common between WAD files and directory lists.

#ifndef INCLUDED_BLOB_LIST_H
#define INCLUDED_BLOB_LIST_H

enum blob_type {
	BLOB_TYPE_FILE,
	BLOB_TYPE_DIR,
	BLOB_TYPE_LUMP,
	BLOB_TYPE_WAD,
};

struct blob_list_entry {
	enum blob_type type;
	char *name;
};

struct blob_list {
	char *path;
	char *parent_dir, *name;
	const struct blob_list_entry *(*get_entry)(
		struct blob_list *p, unsigned int i);
	const char *(*get_entry_path)(struct blob_list *l, unsigned int idx);
	void (*free)(struct blob_list *bl);
};

void BL_SetPathFields(void *bl, const char *path);
void BL_FreeList(void *bl);

#endif /* #ifndef INCLUDED_BLOB_LIST_H */

