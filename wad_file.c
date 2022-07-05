
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "vfile.h"
#include "wad_file.h"

struct wad_file {
	VFILE *vfs;
	struct wad_file_entry *directory;
	int num_lumps;
};

struct wad_file *W_OpenFile(const char *file)
{
	struct wad_file *result;
	struct wad_file_header hdr;
	FILE *fs;
	VFILE *vfs;

	fs = fopen(file, "r");
	if (fs == NULL) {
		return NULL;
	}

	vfs = vfwrapfile(fs);

	assert(vfread(&hdr, sizeof(struct wad_file_header), 1, vfs) == 1);
	assert(!strncmp(hdr.id, "IWAD", 4) ||
	       !strncmp(hdr.id, "PWAD", 4));
	assert(vfseek(vfs, hdr.table_offset) == 0);

	result = calloc(1, sizeof(struct wad_file));
	assert(result != NULL);
	result->vfs = vfs;
	result->num_lumps = hdr.num_lumps;
	result->directory = calloc(hdr.num_lumps, sizeof(struct wad_file_entry));
	assert(result->directory != NULL);
	assert(vfread(result->directory, sizeof(struct wad_file_entry),
	              hdr.num_lumps, vfs) == hdr.num_lumps);

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
	vfclose(f->vfs);
	free(f);
}

