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
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>

#include "common.h"
#include "dialog.h"
#include "vfile.h"
#include "wad_file.h"

#define WAD_FILE_ENTRY_LEN  16

struct wad_file {
	VFILE *vfs;
	struct wad_file_header header;
	struct wad_file_entry *directory;
	int num_lumps;

	// We can only read/write a single lump at once.
	VFILE *current_lump;
	unsigned int current_lump_index;

	// Location of old WAD directory that we can roll back to
	// while writing a new lump, to keep the WAD in a consistent
	// state. If rollback_header.table_offset=0 then there is
	// no old directory.
	struct wad_file_header rollback_header;

	// File offset of the most recently written lump. If this is zero then
	// no lump has been written since the WAD was opened. This is used for
	// an optimization where if we write the same lump repeatedly then we
	// overwrite the previous data.
	unsigned int last_lump_pos;

	// Call to W_WriteDirectory needed.
	bool dirty;
};

static void ReadLumpHeader(struct wad_file *wad, unsigned int lump_index)
{
	size_t bytes = min(wad->directory[lump_index].size, LUMP_HEADER_LEN);
	assert(vfseek(wad->vfs, wad->directory[lump_index].position,
	              SEEK_SET) == 0);
	assert(vfread(&wad->directory[lump_index].lump_header,
	              1, bytes, wad->vfs) == bytes);
}

static uint64_t NewSerialNo(void)
{
	static uint64_t serial_no = 0x800000;
	uint64_t result = serial_no;
	++serial_no;
	return result;
}

// Just creates an empty WAD file.
bool W_CreateFile(const char *filename)
{
	uint8_t zerobuf[sizeof(struct wad_file_header)];
	struct wad_file *wf;
	FILE *fs;

	fs = fopen(filename, "w+");
	if (fs == NULL) {
		return false;
	}

	assert(fwrite(zerobuf, sizeof(zerobuf), 1, fs) == 1);

	wf = checked_calloc(1, sizeof(struct wad_file));
	wf->vfs = vfwrapfile(fs);
	wf->dirty = true;
	memcpy(wf->header.id, "PWAD", 4);
	W_WriteDirectory(wf);
	W_CloseFile(wf);

	return true;
}

static void SwapHeader(struct wad_file_header *hdr)
{
	SwapLE32(&hdr->num_lumps);
	SwapLE32(&hdr->table_offset);
}

static void SwapEntry(struct wad_file_entry *entry)
{
	SwapLE32(&entry->position);
	SwapLE32(&entry->size);
}

struct wad_file *W_OpenFile(const char *filename)
{
	struct wad_file *result;
	FILE *fs;
	int i;
	VFILE *vfs;

	fs = fopen(filename, "r+");
	if (fs == NULL) {
		return NULL;
	}

	vfs = vfwrapfile(fs);

	result = checked_calloc(1, sizeof(struct wad_file));
	result->vfs = vfs;
	result->rollback_header.table_offset = 0;
	result->last_lump_pos = 0;

	if (vfread(&result->header,
	           sizeof(struct wad_file_header), 1, vfs) != 1
	 || (strncmp(result->header.id, "IWAD", 4) != 0
	  && strncmp(result->header.id, "PWAD", 4) != 0)) {
		W_CloseFile(result);
		return NULL;
	}

	SwapHeader(&result->header);

	result->num_lumps = result->header.num_lumps;
	assert(vfseek(vfs, result->header.table_offset, SEEK_SET) == 0);
	result->directory = checked_calloc(
		result->num_lumps, sizeof(struct wad_file_entry));
	for (i = 0; i < result->num_lumps; i++) {
		struct wad_file_entry *ent = &result->directory[i];
		if (vfread(&ent->position, 4, 1, vfs) != 1
		 || vfread(&ent->size, 4, 1, vfs) != 1
		 || vfread(&ent->name, 8, 1, vfs) != 1) {
			W_CloseFile(result);
			return NULL;
		}
		SwapEntry(ent);
		ent->serial_no = NewSerialNo();
	}

	// Read and save the first few bytes of every lump. This contains
	// enough information that we can give a basic summary of several
	// common lump types, eg. demos, graphics, MID/MUS.
	for (i = 0; i < result->num_lumps; i++) {
		ReadLumpHeader(result, i);
	}

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
	free(f);
}

void W_AddEntries(struct wad_file *f, unsigned int before_index,
                  unsigned int count)
{
	unsigned int i;
	struct wad_file_entry *ent;

	assert(before_index <= f->num_lumps);

	// We need to rearrange both the WAD directory and the lump headers
	// array to make room for the new entries.
	f->directory = realloc(f->directory,
	    (f->num_lumps + count) * sizeof(struct wad_file_entry));
	memmove(&f->directory[before_index + count],
	        &f->directory[before_index],
	        (f->num_lumps - before_index) * sizeof(struct wad_file_entry));

	f->num_lumps += count;

	for (i = 0; i < count; i++) {
		ent = &f->directory[before_index + i];
		ent->position = 0;
		ent->size = 0;
		ent->serial_no = NewSerialNo();
		snprintf(ent->name, 8, "UNNAMED");
		memset(&ent->lump_header, 0, LUMP_HEADER_LEN);
	}
	f->dirty = true;
}

void W_DeleteEntry(struct wad_file *f, unsigned int index)
{
	assert(index < f->num_lumps);
	memmove(&f->directory[index], &f->directory[index + 1],
	        (f->num_lumps - index - 1) * sizeof(struct wad_file_entry));
	--f->num_lumps;
	f->dirty = true;
}

void W_SetLumpName(struct wad_file *f, unsigned int index, const char *name)
{
	unsigned int i;
	assert(index < f->num_lumps);
	for (i = 0; i < 8; i++) {
		f->directory[index].name[i] = toupper(name[i]);
		if (name[i] == '\0') {
			break;
		}
	}
	f->dirty = true;
}

size_t W_ReadLumpHeader(struct wad_file *f, unsigned int index,
                        uint8_t *buf, size_t buf_len)
{
	assert(index < f->num_lumps);
	buf_len = min(buf_len, min(LUMP_HEADER_LEN, f->directory[index].size));
	memcpy(buf, &f->directory[index].lump_header, buf_len);
	return buf_len;
}

static void WriteHeader(struct wad_file *f)
{
	struct wad_file_header hdr = f->header;
	SwapHeader(&hdr);
	assert(!vfseek(f->vfs, 0, SEEK_SET));
	assert(vfwrite(&hdr, sizeof(struct wad_file_header), 1, f->vfs) == 1);
}

// MaybeRollbackDirectory rolls back the last write of the WAD header to point
// to the previous directory, if there is such a directory. Returns 1 if a
// rollback was performed.
static int MaybeRollbackDirectory(struct wad_file *f)
{
	unsigned int truncate_at;

	if (f->rollback_header.table_offset == 0) {
		return 0;
	}

	truncate_at = f->header.table_offset;

	f->header = f->rollback_header;
	WriteHeader(f);
	vfsync(f->vfs);
	f->rollback_header.table_offset = 0;

	// Truncate off the new directory now that we no longer point at it.
	vfseek(f->vfs, truncate_at, SEEK_SET);
	vftruncate(f->vfs);

	return 1;
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

	f->dirty = true;

	ReadLumpHeader(f, f->current_lump_index);
}

VFILE *W_OpenLumpRewrite(struct wad_file *f, unsigned int lump_index)
{
	VFILE *result;
	struct wad_file_entry *wadent;
	long start;

	assert(lump_index < f->num_lumps);
	assert(f->current_lump == NULL);
	wadent = &f->directory[lump_index];

	// Every time we write a lump we write the new data to the EOF, then
	// the new directory, then update the WAD header. As an optimization,
	// if we write multiple lumps, each time we roll back the directory to
	// point to the old directory and truncate the new directory off the
	// EOF. This optimization ensures that after writing N lumps, the WAD
	// file will only have one new directory written to it, rather than N
	// copies of the directory. But at each stage the WAD file is always
	// consistent.
	if (MaybeRollbackDirectory(f)
	 && f->last_lump_pos > 0 && wadent->size > 0
	 && f->last_lump_pos == wadent->position) {
		// We performed a rollback. But there's an additional
		// optimization we can still perform. At the tail of the file
		// is now the lump data for the last lump we wrote. If we're
		// writing the same lump again then we can overwrite that, too.
		start = f->last_lump_pos;
		assert(!vfseek(f->vfs, start, SEEK_SET));
	} else {
		assert(!vfseek(f->vfs, 0, SEEK_END));
		start = vftell(f->vfs);
	}

	f->directory[lump_index].position = (unsigned int) start;

	result = vfrestrict(f->vfs, start, -1, 0);
	f->current_lump = result;
	f->current_lump_index = lump_index;
	f->last_lump_pos = start;

	vfonclose(result, WriteLumpClosed, f);

	return result;
}

static void WriteDirectoryCurrentPos(struct wad_file *f)
{
	int i;

	// Save the current directory pointer so we can roll back to it later
	// if we want to overwrite this new directory with an updated one.
	f->rollback_header = f->header;

	f->header.table_offset = (unsigned int) vftell(f->vfs);
	f->header.num_lumps = f->num_lumps;
	for (i = 0; i < f->num_lumps; i++) {
		struct wad_file_entry ent = f->directory[i];
		SwapEntry(&ent);
		assert(vfwrite(&ent.position, 4, 1, f->vfs) == 1 &&
		       vfwrite(&ent.size, 4, 1, f->vfs) == 1 &&
		       vfwrite(&ent.name, 8, 1, f->vfs) == 1);

	}
	vfsync(f->vfs);
	WriteHeader(f);
	f->dirty = false;
}

void W_WriteDirectory(struct wad_file *f)
{
	if (!f->dirty) {
		return;
	}

	// See comment in W_OpenLumpRewrite above. If we are writing a new
	// lump (as opposed to directory modification) then this has already
	// taken place.
	MaybeRollbackDirectory(f);

	// We always write the directory to the end of file.
	assert(vfseek(f->vfs, 0, SEEK_END) == 0);
	WriteDirectoryCurrentPos(f);
}

static uint32_t MinimumWADSize(struct wad_file *f)
{
	size_t result = sizeof(struct wad_file_header)
	              + WAD_FILE_ENTRY_LEN * f->num_lumps;
	int i;

	for (i = 0; i < f->num_lumps; i++) {
		result += f->directory[i].size;
	}

	return result;
}

uint32_t W_NumJunkBytes(struct wad_file *f)
{
	uint32_t min_size = MinimumWADSize(f);
	long curr_size;

	if (vfseek(f->vfs, 0, SEEK_END) != 0) {
		return 0;
	}

	curr_size = vftell(f->vfs);
	if (curr_size < min_size) {
		return 0;
	} else {
		return curr_size - min_size;
	}
}

static bool RewriteAllLumps(struct progress_window *progress, struct wad_file *f)
{
	VFILE *lump;
	uint32_t new_pos;
	int i;

	for (i = 0; i < f->num_lumps; i++) {
		new_pos = vftell(f->vfs);
		lump = W_OpenLump(f, i);
		if (lump == NULL) {
			return false;
		}
		if (vfcopy(lump, f->vfs) != 0) {
			return false;
		}
		vfclose(lump);
		f->directory[i].position = new_pos;
		UI_UpdateProgressWindow(progress, "");
	}

	WriteDirectoryCurrentPos(f);
	return true;
}

bool W_CompactWAD(struct wad_file *f)
{
	struct progress_window progress;
	uint32_t min_size = MinimumWADSize(f);

	// Is file length shorter than the minimum size already? (This
	// can happen if the file was compressed with wadptr)
	if (vfseek(f->vfs, 0, SEEK_END) != 0 || vftell(f->vfs) <= min_size) {
		return false;
	}

	// In compacting the WAD the end goal is to have all lumps at the
	// start of the file (with no gaps), followed by the WAD directory,
	// then the EOF. Step 1 is that we move all the existing data to the
	// end of the file so that it is out of the way. Once this is
	// complete and the directory has been updated to point to the new
	// locations, step 2 is that we move all the data back again to the
	// beginning of the file, then truncate it to the minimum size.

	// TODO: Compacting could be made more effective by reusing the code
	// from wadptr.

	UI_InitProgressWindow(&progress, f->num_lumps * 2, "Compacting WAD");

	// Rewrite the whole file's contents to its end.
	if (!RewriteAllLumps(&progress, f)) {
		return false;
	}

	// Now seek back to the start of file and rewrite everything again.
	if (vfseek(f->vfs, sizeof(struct wad_file_header), SEEK_SET) != 0) {
		return false;
	}

	if (!RewriteAllLumps(&progress, f)) {
		return false;
	}

	// Seek to new EOF, truncate, and we're done.
	if (vfseek(f->vfs,
	           f->header.table_offset + f->num_lumps * WAD_FILE_ENTRY_LEN,
	           SEEK_SET) != 0) {
		return false;
	}
	vftruncate(f->vfs);
	return true;
}
