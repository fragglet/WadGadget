
#include "blob_list.h"
#include "blob_list_pane.h"

struct directory_listing;

struct directory_listing *DIR_ReadDirectory(const char *path);
void DIR_FreeDirectory(struct directory_listing *dir);
const struct blob_list_entry *DIR_GetFile(
	struct directory_listing *dir, unsigned int file_index);
unsigned int DIR_NumFiles(struct directory_listing *dir);
void DIR_RefreshDirectory(struct directory_listing *d);
const char *DIR_GetPath(struct directory_listing *d);

