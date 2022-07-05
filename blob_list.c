
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "blob_list.h"

void BL_SetPathFields(void *_bl, const char *path)
{
	struct blob_list *bl = _bl;
	char *s;
	bl->path = strdup(path);
	assert(bl->path != NULL);
	s = strrchr(path, '/');
	if (s != NULL) {
		bl->parent_dir = strdup(path);
		assert(bl->parent_dir != NULL);
		bl->parent_dir[s - path] = '\0';
		bl->name = strdup(s + 1);
		assert(bl->name != NULL);
	} else {
		bl->parent_dir = NULL;
		bl->name = strdup(path);
		assert(bl->name != NULL);
	}
}

