
#ifndef INCLUDED_BLOB_LIST_H
#define INCLUDED_BLOB_LIST_H

struct blob_list {
	char *parent_dir, *name;
};

void BL_SetPathFields(void *bl, const char *path);

#endif /* #ifndef INCLUDED_BLOB_LIST_H */

