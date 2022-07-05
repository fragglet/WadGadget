
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "vfile.h"
#include "wad_file.h"

struct wad_file {
	struct blob_list bl;
	VFILE *vfs;
	struct wad_file_entry *directory;
	int num_lumps;

	// We can only read/write a single lump at once.
	VFILE *current_lump;
	unsigned int current_lump_index;
};

static const char *GetEntry(struct blob_list *l, unsigned int idx)
{
	static char buf[9];
	struct wad_file *f = (struct wad_file *) l;
	if (idx >= f->num_lumps) {
		return NULL;
	}
	snprintf(buf, sizeof(buf), "%-8s", f->directory[idx].name);
	return buf;
}

static enum blob_type GetEntryType(struct blob_list *l, unsigned int idx)
{
	return BLOB_TYPE_LUMP;
}

static void FreeWadFile(struct blob_list *l)
{
	W_CloseFile((struct wad_file *) l);
}

struct wad_file *W_OpenFile(const char *filename)
{
	struct wad_file *result;
	struct wad_file_header hdr;
	FILE *fs;
	VFILE *vfs;

	fs = fopen(filename, "r");
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
	result->bl.get_entry_str = GetEntry;
	result->bl.get_entry_type = GetEntryType;
	result->bl.free = FreeWadFile;
	result->num_lumps = hdr.num_lumps;
	result->directory = calloc(hdr.num_lumps, sizeof(struct wad_file_entry));
	assert(result->directory != NULL);
	assert(vfread(result->directory, sizeof(struct wad_file_entry),
	              hdr.num_lumps, vfs) == hdr.num_lumps);
	BL_SetPathFields(&result->bl, filename);

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
	if (f->current_lump != NULL) {
		vfclose(f->current_lump);
	}
	vfclose(f->vfs);
	free(f->directory);
	free(f->bl.path);
	free(f->bl.parent_dir);
	free(f->bl.name);
	free(f);
}

static void LumpClosed(VFILE *fs, void *data)
{
	struct wad_file *f = data;

	assert(f->current_lump == fs);
	f->current_lump = NULL;
}

VFILE *W_OpenLump(struct wad_file *f, unsigned int lump_index)
{
	VFILE *result;
	long start, end;

	assert(lump_index < f->num_lumps);
	assert(f->current_lump == NULL);

	start = f->directory[lump_index].position;
	end = start + f->directory[lump_index].size;
	result = vfrestrict(f->vfs, start, end, 1);

	f->current_lump = result;
	f->current_lump_index = lump_index;
	vfonclose(result, LumpClosed, f);

	return result;
}

