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
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "common.h"
#include "stringlib.h"
#include "fs/vfs.h"

void VFS_ClearSet(struct file_set *l)
{
	l->num_entries = 0;
}

static unsigned int SearchForTag(struct file_set *l, uint64_t serial_no)
{
	unsigned int min = 0, max = l->num_entries;

	while (min < max) {
		unsigned int midpoint;
		uint64_t test_serial;
		midpoint = (min + max) / 2;
		test_serial = l->entries[midpoint];
		if (serial_no == test_serial) {
			return midpoint;
		} else if (serial_no > test_serial) {
			min = midpoint + 1;
		} else {
			max = midpoint;
		}
	}

	return min;
}

void VFS_AddToSet(struct file_set *l, uint64_t serial_no)
{
	unsigned int entries_index = SearchForTag(l, serial_no);

	// Already in list?
	if (entries_index < l->num_entries
	 && l->entries[entries_index] == serial_no) {
		return;
	}

	l->entries = checked_realloc(l->entries,
		sizeof(uint64_t) * (l->num_entries + 1));
	memmove(&l->entries[entries_index + 1], &l->entries[entries_index],
	        sizeof(uint64_t) * (l->num_entries - entries_index));
	l->entries[entries_index] = serial_no;
	++l->num_entries;
}

static bool GlobMatch(const char *pattern, const char *s)
{
	switch (*pattern) {
	case '*':
		return GlobMatch(pattern + 1, s)
		    || (*s != '\0' && GlobMatch(pattern, s + 1));
	case '\0':
		return *s == '\0';
	case '?':
		return *s != '\0' && GlobMatch(pattern + 1, s + 1);
	default:
		return tolower(*s) == tolower(*pattern)
		    && GlobMatch(pattern + 1, s + 1);
	}
}

// Mark all entries matching glob pattern. Returns first entry matched.
struct directory_entry *VFS_AddGlobToSet(
	struct directory *dir, struct file_set *l, const char *glob)
{
	struct directory_entry *ent, *result = NULL;
	int i = 0;

	for (i = 0; i < dir->num_entries; ++i) {
		ent = &dir->entries[i];
		if (ent->type != FILE_TYPE_DIR && GlobMatch(glob, ent->name)) {
			VFS_AddToSet(l, ent->serial_no);
			if (result == NULL) {
				result = ent;
			}
		}
	}
	return result;
}

void VFS_RemoveFromSet(struct file_set *l, uint64_t serial_no)
{
	unsigned int entries_index = SearchForTag(l, serial_no);

	// Not in list?
	if (entries_index >= l->num_entries
	 || l->entries[entries_index] != serial_no) {
		return;
	}

	memmove(&l->entries[entries_index], &l->entries[entries_index + 1],
	        sizeof(uint64_t) * (l->num_entries - 1 - entries_index));
	--l->num_entries;
}

bool VFS_SetHas(struct file_set *l, uint64_t serial_no)
{
	unsigned int entries_index = SearchForTag(l, serial_no);

	return entries_index < l->num_entries
	    && l->entries[entries_index] == serial_no;
}


void VFS_CopySet(struct file_set *to, struct file_set *from)
{
	to->num_entries = from->num_entries;
	to->entries = checked_calloc(to->num_entries, sizeof(uint64_t));
	memcpy(to->entries, from->entries,
	       to->num_entries * sizeof(uint64_t));
}

void VFS_FreeSet(struct file_set *set)
{
	free(set->entries);
	set->entries = NULL;
	set->num_entries = 0;
}

struct directory_entry *VFS_IterateSet(struct directory *dir,
                                       struct file_set *set, int *idx)
{
	while (*idx < dir->num_entries) {
		struct directory_entry *ent = &dir->entries[*idx];
		++*idx;
		if (VFS_SetHas(set, ent->serial_no)) {
			return ent;
		}
	}

	return NULL;
}

void VFS_DescribeSet(struct directory *dir, struct file_set *set,
                     char *buf, size_t buf_len)
{
	if (set->num_entries == 0) {
		snprintf(buf, buf_len, "nothing");
	} if (set->num_entries == 1) {
		struct directory_entry *ent;
		ent = VFS_EntryBySerial(dir, set->entries[0]);
		if (ent == NULL) {
			snprintf(buf, buf_len, "nothing?");
			return;
		}
		snprintf(buf, buf_len, "'%s'", ent->name);
	} else {
		assert(dir->directory_funcs->describe_entries != NULL);
		dir->directory_funcs->describe_entries(
			buf, buf_len, set->num_entries);
	}
}

