
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "blob_list.h"

void BL_SetPathFields(void *_bl, const char *path)
{
	struct blob_list *bl = _bl;
	char *s;
	bl->path = checked_strdup(path);
	s = strrchr(path, '/');
	if (s != NULL) {
		if (s > path) {
			bl->parent_dir = checked_strdup(path);
			bl->parent_dir[s - path] = '\0';
		} else {
			bl->parent_dir = checked_strdup("/");
		}
		bl->name = checked_strdup(s + 1);
	} else {
		bl->parent_dir = NULL;
		bl->name = checked_strdup(path);
	}
}

void BL_FreeList(void *ptr)
{
	struct blob_list *bl = ptr;
	bl->free(bl);
}

