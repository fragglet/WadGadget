
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "wad_file.h"

struct wad_file {
	FILE *fs;
	struct wad_file_entry *directory;
	int num_lumps;
};

struct wad_file *W_OpenFile(const char *file)
{
	struct wad_file *result;
	struct wad_file_header hdr;
	FILE *fs;

	fs = fopen(file, "r");
	if (fs == NULL) {
		return NULL;
	}

	assert(fread(&hdr, sizeof(struct wad_file_header), 1, fs) == 1);
	assert(!strncmp(hdr.id, "IWAD", 4) ||
	       !strncmp(hdr.id, "PWAD", 4));
	assert(fseek(fs, hdr.table_offset, SEEK_SET) == 0);

	result = calloc(1, sizeof(struct wad_file));
	assert(result != NULL);
	result->fs = fs;
	result->num_lumps = hdr.num_lumps;
	result->directory = calloc(hdr.num_lumps, sizeof(struct wad_file_entry));
	assert(result->directory != NULL);
	assert(fread(result->directory, sizeof(struct wad_file_entry),
	             hdr.num_lumps, fs) == hdr.num_lumps);

	return result;
}

struct wad_file_entry *W_GetDirectory(struct wad_file *f)
{
	return f->directory;
}

unsigned int W_NumLumps(struct wad_file *f)
{
	return f->num_lumps;
}

void W_CloseFile(struct wad_file *f)
{
	fclose(f->fs);
	free(f);
}

