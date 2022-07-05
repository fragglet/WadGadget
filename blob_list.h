
// For want of a better name.
// Things that are in common between WAD files and directory lists.

#ifndef INCLUDED_BLOB_LIST_H
#define INCLUDED_BLOB_LIST_H

struct blob_list {
	char *path;
	char *parent_dir, *name;
};

void BL_SetPathFields(void *bl, const char *path);

#endif /* #ifndef INCLUDED_BLOB_LIST_H */

