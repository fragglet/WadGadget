
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "vfile.h"
#include "wad_file.h"

struct wad_file {
	struct blob_list bl;
	VFILE *vfs;
	struct wad_file_header header;
	struct wad_file_entry *directory;
	int num_lumps;

	// We can only read/write a single lump at once.
	VFILE *current_lump;
	unsigned int current_lump_index;
};

static const struct blob_list_entry *GetEntry(
	struct blob_list *l, unsigned int idx)
{
	static char buf[9];
	static struct blob_list_entry result;
	struct wad_file *f = (struct wad_file *) l;
	if (idx >= f->num_lumps) {
		return NULL;
	}
	snprintf(buf, sizeof(buf), "%-8s", f->directory[idx].name);
	result.name = buf;
	result.type = BLOB_TYPE_LUMP;
	return &result;
}

static void FreeWadFile(struct blob_list *l)
{
	W_CloseFile((struct wad_file *) l);
}

struct wad_file *W_OpenFile(const char *filename)
{
	struct wad_file *result;
	FILE *fs;
	VFILE *vfs;

	fs = fopen(filename, "r");
	if (fs == NULL) {
		return NULL;
	}

	vfs = vfwrapfile(fs);

	result = checked_calloc(1, sizeof(struct wad_file));
	result->vfs = vfs;
	result->bl.get_entry = GetEntry;
	result->bl.free = FreeWadFile;
	BL_SetPathFields(&result->bl, filename);

	assert(vfread(&result->header,
	              sizeof(struct wad_file_header), 1, vfs) == 1);
	assert(!strncmp(result->header.id, "IWAD", 4) ||
	       !strncmp(result->header.id, "PWAD", 4));

	result->num_lumps = result->header.num_lumps;
	assert(vfseek(vfs, result->header.table_offset, SEEK_SET) == 0);
	result->directory = checked_calloc(
		result->num_lumps, sizeof(struct wad_file_entry));
	assert(vfread(result->directory, sizeof(struct wad_file_entry),
	              result->num_lumps, vfs) == result->num_lumps);

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

void W_AddEntries(struct wad_file *f, unsigned int after_index,
                  unsigned int count)
{
	unsigned int i;
	struct wad_file_entry *ent;

	assert(after_index <= f->num_lumps);
	f->directory = realloc(f->directory,
	    (f->num_lumps + count) * sizeof(struct wad_file_entry));
	memmove(&f->directory[after_index + count],
	        &f->directory[after_index],
	        (f->num_lumps - after_index) * sizeof(struct wad_file_entry));
	f->num_lumps += count;
	for (i = 0; i < count; i++) {
		ent = &f->directory[after_index + i];
		ent->position = 0;
		ent->size = 0;
		snprintf(ent->name, 8, "UNNAMED");
	}
}

void W_DeleteEntry(struct wad_file *f, unsigned int index)
{
	assert(index < f->num_lumps);
	memmove(&f->directory[index], &f->directory[index + 1],
	        (f->num_lumps - index - 1) * sizeof(struct wad_file_entry));
	--f->num_lumps;
}

void W_SetLumpName(struct wad_file *f, unsigned int index, char *name)
{
	unsigned int i;
	assert(index < f->num_lumps);
	for (i = 0; i < 8; i++) {
		f->directory[index].name[i] = toupper(name[i]);
		if (name[i] == '\0') {
			break;
		}
	}
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

static void WriteLumpClosed(VFILE *fs, void *data)
{
	struct wad_file *f = data;
	long size;

	assert(f->current_lump == fs);
	f->current_lump = NULL;

	// New size of lump is the offset within the restricted VFILE.
	size = vftell(fs);
	assert(f->current_lump_index < f->num_lumps);
	f->directory[f->current_lump_index].size = (unsigned int) size;
}

VFILE *W_OpenLumpRewrite(struct wad_file *f, unsigned int lump_index)
{
	VFILE *result;
	long start;

	assert(lump_index < f->num_lumps);
	assert(f->current_lump == NULL);

	// We always write at the end of file.
	assert(!vfseek(f->vfs, 0, SEEK_END));
	start = vftell(f->vfs);
	f->directory[lump_index].position = (unsigned int) start;

	result = vfrestrict(f->vfs, start, 0, 0);
	f->current_lump = result;
	f->current_lump_index = lump_index;

	vfonclose(result, WriteLumpClosed, f);

	return result;
}

void W_WriteDirectory(struct wad_file *f)
{
	// We always write the directory to the end of file.
	assert(!vfseek(f->vfs, 0, SEEK_END));
	f->header.table_offset = (unsigned int) vftell(f->vfs);
	f->header.num_lumps = f->num_lumps;
	assert(vfwrite(f->directory, sizeof(struct wad_file_entry),
	               f->num_lumps, f->vfs) == f->num_lumps);
	vfsync(f->vfs);

	assert(!vfseek(f->vfs, 0, SEEK_SET));
	assert(vfread(&f->header,
	              sizeof(struct wad_file_header), 1, f->vfs) == 1);
}

