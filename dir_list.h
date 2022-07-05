
#include "blob_list.h"
#include "list_pane.h"

struct directory_listing;

struct directory_entry {
	const char *filename;
	// TODO: Others
};

struct directory_listing *DIR_ReadDirectory(const char *path);
void DIR_FreeDirectory(struct directory_listing *dir);
const struct directory_entry *DIR_GetFile(
	struct directory_listing *dir, unsigned int file_index);

