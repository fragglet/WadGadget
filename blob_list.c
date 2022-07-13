
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

static unsigned int SearchForTag(struct blob_tag_list *l, unsigned int index)
{
	unsigned int min = 0, max = l->num_entries;

	while (min < max) {
		unsigned int midpoint, test_index;
		midpoint = (min + max) / 2;
		test_index = l->entries[midpoint];
		if (index == test_index) {
			return midpoint;
		} else if (index > test_index) {
			min = midpoint + 1;
		} else {
			max = midpoint;
		}
	}

	return min;
}

void BL_AddTag(struct blob_tag_list *l, unsigned int index)
{
	unsigned int entries_index = SearchForTag(l, index);

	// Already in list?
	if (entries_index < l->num_entries
	 && l->entries[entries_index] == index) {
		return;
	}

	l->entries = checked_realloc(l->entries,
		sizeof(unsigned int) * (l->num_entries + 1));
	memmove(&l->entries[entries_index + 1], &l->entries[entries_index],
	        sizeof(unsigned int) * (l->num_entries - entries_index));
	l->entries[entries_index] = index;
	++l->num_entries;
}

void BL_RemoveTag(struct blob_tag_list *l, unsigned int index)
{
	unsigned int entries_index = SearchForTag(l, index);

	// Not in list?
	if (entries_index >= l->num_entries
	 || l->entries[entries_index] != index) {
		return;
	}

	memmove(&l->entries[entries_index], &l->entries[entries_index + 1],
	        sizeof(unsigned int) * (l->num_entries - 1 - entries_index));
	--l->num_entries;
}

int BL_IsTagged(struct blob_tag_list *l, unsigned int index)
{
	unsigned int entries_index = SearchForTag(l, index);

	return entries_index < l->num_entries
	    && l->entries[entries_index] == index;
}

void BL_HandleInsert(struct blob_tag_list *l, unsigned int index)
{
	unsigned int entries_index = SearchForTag(l, index);
	unsigned int i;

	// All entries higher than the index shift up by one.
	for (i = entries_index; i < l->num_entries; i++) {
		l->entries[i]++;
	}
}

void BL_ClearTags(struct blob_tag_list *l)
{
	l->num_entries = 0;
}

void BL_HandleDelete(struct blob_tag_list *l, unsigned int index)
{
	unsigned int entries_index;
	unsigned int i;

	// If it's tagged, remove it from the list.
	BL_RemoveTag(l, index);

	entries_index = SearchForTag(l, index);

	// All entries higher than the index shift down by one.
	for (i = entries_index; i < l->num_entries; i++) {
		l->entries[i]--;
	}
}
