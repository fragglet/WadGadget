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

#define UNDO_LEVELS         50
#define WAD_FILE_ENTRY_LEN  16

struct wad_file {
	VFILE *vfs;
	struct wad_file_entry *directory;
	int num_lumps;

	// We can only read/write a single lump at once.
	VFILE *current_lump;
	unsigned int current_lump_index;

	// We support Undo by keeping multiple copies of the WAD header.
	// Every time anything in the file changes, we write a new WAD
	// directory. To undo, we simply revert the WAD header to point
	// back to the old directory. Redo is implemented in the same way.
	// This does mean that a WAD can become very fragmented over time
	// as we keep writing new directories. When this happens, the
	// user can use W_CompactWAD() to remove them.
	struct wad_file_header headers[UNDO_LEVELS];
	int current_header, num_headers;

	// Call to W_CommitChanges needed.
	bool dirty;
};

static void ReadLumpHeader(struct wad_file *wad, struct wad_file_entry *ent)
{
	size_t bytes = min(ent->size, LUMP_HEADER_LEN);
	assert(vfseek(wad->vfs, ent->position, SEEK_SET) == 0);
	assert(vfread(&ent->lump_header, 1, bytes, wad->vfs) == bytes);
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
	wf->num_headers = 1;
	wf->current_header = 0;
	memcpy(wf->headers[0].id, "PWAD", 4);
	W_CommitChanges(wf);
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

// Read WAD directory based on index in wf->headers[wf->current_header].
// If there is a current directory, it is replaced.
#define LOOKAHEAD 10
static bool ReadDirectory(struct wad_file *wf)
{
	struct wad_file_entry *new_directory;
	size_t new_num_lumps;
	int i, j, k, old_lump_index;

	new_num_lumps = wf->headers[wf->current_header].num_lumps;
	assert(vfseek(wf->vfs, wf->headers[wf->current_header].table_offset,
	              SEEK_SET) == 0);
	new_directory = checked_calloc(
		new_num_lumps, sizeof(struct wad_file_entry));

	for (i = 0, j = 0; i < new_num_lumps; i++) {
		struct wad_file_entry *ent = &new_directory[i], *oldent;
		if (vfread(&ent->position, 4, 1, wf->vfs) != 1
		 || vfread(&ent->size, 4, 1, wf->vfs) != 1
		 || vfread(&ent->name, 8, 1, wf->vfs) != 1) {
			free(new_directory);
			return false;
		}
		SwapEntry(ent);

		// Try to map to a map in the old directory; if found,
		// we preserve the serial number and do not re-read
		// the header bytes. We only look several lumps ahead;
		// in the worst case, we lose all serial numbers and
		// have to re-read all lump headers.
		old_lump_index = -1;
		oldent = NULL;
		for (k = j; k < min(j + LOOKAHEAD, wf->num_lumps); k++) {
			oldent = &wf->directory[k];
			if (ent->position == oldent->position
			 && ent->size == oldent->size
			 && !strncmp(ent->name, oldent->name, 8)) {
				old_lump_index = k;
				break;
			}
		}
		if (old_lump_index == -1) {
			long old_pos = vftell(wf->vfs);
			ent->serial_no = NewSerialNo();
			ReadLumpHeader(wf, ent);
			vfseek(wf->vfs, old_pos, SEEK_SET);
		} else {
			// We got a match!
			ent->serial_no = oldent->serial_no;
			memcpy(ent->lump_header, oldent->lump_header,
			       LUMP_HEADER_LEN);
			j = old_lump_index + 1;
		}
	}

	free(wf->directory);
	wf->directory = new_directory;
	wf->num_lumps = new_num_lumps;
	return true;
}

struct wad_file *W_OpenFile(const char *filename)
{
	struct wad_file *result;
	FILE *fs;
	VFILE *vfs;

	fs = fopen(filename, "r+");
	if (fs == NULL) {
		return NULL;
	}

	vfs = vfwrapfile(fs);

	result = checked_calloc(1, sizeof(struct wad_file));
	result->vfs = vfs;
	result->directory = NULL;
	result->num_lumps = 0;
	result->num_headers = 1;
	result->current_header = 0;

	if (vfread(&result->headers[0],
	           sizeof(struct wad_file_header), 1, vfs) != 1
	 || (strncmp(result->headers[0].id, "IWAD", 4) != 0
	  && strncmp(result->headers[0].id, "PWAD", 4) != 0)) {
		W_CloseFile(result);
		return NULL;
	}

	SwapHeader(&result->headers[0]);

	if (!ReadDirectory(result)) {
		W_CloseFile(result);
		result = NULL;
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
	W_DeleteEntries(f, index, 1);
}

void W_DeleteEntries(struct wad_file *f, unsigned int index, unsigned int cnt)
{
	assert(index <= f->num_lumps);
	assert(cnt <= f->num_lumps);
	assert(index + cnt <= f->num_lumps);
	memmove(&f->directory[index], &f->directory[index + cnt],
	        (f->num_lumps - index - cnt) * sizeof(struct wad_file_entry));
	f->num_lumps -= cnt;
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
	struct wad_file_header hdr = f->headers[f->current_header];
	SwapHeader(&hdr);
	assert(!vfseek(f->vfs, 0, SEEK_SET));
	assert(vfwrite(&hdr, sizeof(struct wad_file_header), 1, f->vfs) == 1);
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

	ReadLumpHeader(f, &f->directory[f->current_lump_index]);
}

VFILE *W_OpenLumpRewrite(struct wad_file *f, unsigned int lump_index)
{
	VFILE *result;
	long start;

	assert(lump_index < f->num_lumps);
	assert(f->current_lump == NULL);

	assert(!vfseek(f->vfs, 0, SEEK_END));
	start = vftell(f->vfs);

	f->directory[lump_index].position = (unsigned int) start;

	result = vfrestrict(f->vfs, start, -1, 0);
	f->current_lump = result;
	f->current_lump_index = lump_index;

	vfonclose(result, WriteLumpClosed, f);

	return result;
}

static void WriteDirectoryCurrentPos(struct wad_file *f)
{
	int i;

	// Writing the new directory will create a new undo level.
	// Check we don't overflow.
	if (f->current_header + 1 >= UNDO_LEVELS) {
		memmove(&f->headers[0], &f->headers[1],
		        sizeof(struct wad_file_header) * (UNDO_LEVELS - 1));
		f->current_header = UNDO_LEVELS - 1;
		--f->num_headers;
	}
	++f->current_header;
	f->headers[f->current_header] = f->headers[f->current_header - 1];
	f->headers[f->current_header].table_offset =
		(unsigned int) vftell(f->vfs);
	f->headers[f->current_header].num_lumps = f->num_lumps;

	// Any "redo" is impossible now.
	f->num_headers = f->current_header + 1;

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

void W_CommitChanges(struct wad_file *f)
{
	if (!f->dirty) {
		return;
	}

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
	uint32_t new_eof;

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
	new_eof = f->headers[f->current_header].table_offset
	        + f->num_lumps * WAD_FILE_ENTRY_LEN;
	if (vfseek(f->vfs, new_eof, SEEK_SET) != 0) {
		return false;
	}
	vftruncate(f->vfs);

	// We cannot undo any more.
	memmove(&f->headers[0], &f->headers[f->current_header],
	        sizeof(struct wad_file_header));
	f->current_header = 0;
	f->num_headers = 1;
	return true;
}

int W_CanUndo(struct wad_file *wf)
{
	return wf->current_header;
}

bool W_Undo(struct wad_file *wf, unsigned int levels)
{
	assert(levels <= W_CanUndo(wf));
	wf->current_header -= levels;
	if (!ReadDirectory(wf)) {
		wf->current_header += levels;
		return false;
	}
	WriteHeader(wf);
	wf->dirty = false;
	return true;
}

int W_CanRedo(struct wad_file *wf)
{
	return wf->num_headers - wf->current_header - 1;
}

bool W_Redo(struct wad_file *wf, unsigned int levels)
{
	assert(levels <= W_CanRedo(wf));
	wf->current_header += levels;
	if (!ReadDirectory(wf)) {
		wf->current_header -= levels;
		return false;
	}
	WriteHeader(wf);
	wf->dirty = false;
	return true;
}
