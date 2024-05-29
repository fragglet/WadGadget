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

#define CURR_REVISION(wf) \
	(&(wf)->revisions[(wf)->current_revision])
#define CURR_HEADER(wf) (&CURR_REVISION(wf)->header)

struct wad_revision {
	struct wad_file_header header;
	long eof;
};

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
	// as we keep writing new directories. When this happens, the user
	// can use W_CompactWAD() to remove them.
	struct wad_revision revisions[UNDO_LEVELS];
	int current_revision, num_revisions;

	// Current position to start writing any new data. This is usually
	// at the EOF, but if we undid a previous change we may be
	// overwriting previously written data.
	long write_pos;

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
	wf->num_revisions = 1;
	wf->current_revision = 0;
	memcpy(CURR_HEADER(wf)->id, "PWAD", 4);
	wf->revisions[0].eof = sizeof(struct wad_file_header);
	wf->write_pos = sizeof(struct wad_file_header);
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

// Read WAD directory based on CURR_HEADER(wf)->tablet_offet.
// If there is a current directory, it is replaced.
#define LOOKAHEAD 10
static bool ReadDirectory(struct wad_file *wf)
{
	struct wad_file_entry *new_directory;
	size_t new_num_lumps;
	int i, j, k, old_lump_index;

	new_num_lumps = CURR_HEADER(wf)->num_lumps;
	assert(vfseek(wf->vfs, CURR_HEADER(wf)->table_offset,
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
	result->num_revisions = 1;
	result->current_revision = 0;

	if (vfread(CURR_HEADER(result),
	           sizeof(struct wad_file_header), 1, vfs) != 1
	 || (strncmp(CURR_HEADER(result)->id, "IWAD", 4) != 0
	  && strncmp(CURR_HEADER(result)->id, "PWAD", 4) != 0)) {
		W_CloseFile(result);
		return NULL;
	}

	SwapHeader(CURR_HEADER(result));

	if (!ReadDirectory(result)) {
		W_CloseFile(result);
		return NULL;
	}

	if (vfseek(result->vfs, 0, SEEK_END) != 0) {
		W_CloseFile(result);
		return NULL;
	}
	result->revisions[0].eof = vftell(result->vfs);
	result->write_pos = result->revisions[0].eof;

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
	// After closing the file we lose all ability to redo (or undo, for
	// that matter). Any data after the EOF for the current revision
	// can therefore be safely discarded. The most important reason for
	// doing this is that if we open a file, make some changes and then
	// undo them all, the file will be precisely restored to its
	// original contents.
	if (vfseek(f->vfs, CURR_REVISION(f)->eof, SEEK_SET) == 0) {
		vftruncate(f->vfs);
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
	struct wad_file_header hdr = *CURR_HEADER(f);
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
	struct wad_file_entry *ent;
	struct wad_file *f = data;
	long size;

	assert(f->current_lump == fs);
	f->current_lump = NULL;

	// New size of lump is the offset within the restricted VFILE.
	size = vftell(fs);
	assert(f->current_lump_index < f->num_lumps);
	ent = &f->directory[f->current_lump_index];
	ent->size = (unsigned int) size;
	f->write_pos = ent->position + ent->size;
	f->dirty = true;

	ReadLumpHeader(f, ent);
}

VFILE *W_OpenLumpRewrite(struct wad_file *f, unsigned int lump_index)
{
	VFILE *result;

	assert(lump_index < f->num_lumps);
	assert(f->current_lump == NULL);

	assert(!vfseek(f->vfs, f->write_pos, SEEK_SET));
	f->directory[lump_index].position = (unsigned int) f->write_pos;

	result = vfrestrict(f->vfs, f->write_pos, -1, 0);
	f->current_lump = result;
	f->current_lump_index = lump_index;

	vfonclose(result, WriteLumpClosed, f);

	return result;
}

static void WriteDirectory(struct wad_file *f)
{
	int i;

	// Writing the new directory will create a new undo level.
	// Check we don't overflow.
	if (f->current_revision + 1 >= UNDO_LEVELS) {
		memmove(&f->revisions[0], &f->revisions[1],
		        sizeof(struct wad_revision) * (UNDO_LEVELS - 1));
		f->current_revision = UNDO_LEVELS - 1;
		--f->num_revisions;
	}

	assert(vfseek(f->vfs, f->write_pos, SEEK_SET) == 0);

	for (i = 0; i < f->num_lumps; i++) {
		struct wad_file_entry ent = f->directory[i];
		SwapEntry(&ent);
		assert(vfwrite(&ent.position, 4, 1, f->vfs) == 1 &&
		       vfwrite(&ent.size, 4, 1, f->vfs) == 1 &&
		       vfwrite(&ent.name, 8, 1, f->vfs) == 1);

	}

	++f->current_revision;
	*CURR_REVISION(f) = f->revisions[f->current_revision - 1];
	CURR_HEADER(f)->table_offset = f->write_pos;
	CURR_HEADER(f)->num_lumps = f->num_lumps;
	f->write_pos = vftell(f->vfs);

	// Save the current EOF. If we roll back to this revision later,
	// we can truncate the file here.
	CURR_REVISION(f)->eof = f->write_pos;

	vfsync(f->vfs);
	WriteHeader(f);
	f->dirty = false;

	// Any "redo" is impossible now.
	f->num_revisions = f->current_revision + 1;
}

void W_SwapEntries(struct wad_file *f, unsigned int l1, unsigned int l2)
{
	struct wad_file_entry tmp;

	if (l1 == l2) {
		return;
	}
	assert(l1 < f->num_lumps);
	assert(l2 < f->num_lumps);
	tmp = f->directory[l1];
	f->directory[l1] = f->directory[l2];
	f->directory[l2] = tmp;
	f->dirty = true;
}

void W_CommitChanges(struct wad_file *f)
{
	if (!f->dirty) {
		return;
	}

	WriteDirectory(f);
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

	// Note that we do not use he actual current file size. All
	// data past the EOF of the current revision will be truncated
	// when the file is closed anyway, so they don't count.
	curr_size = CURR_REVISION(f)->eof;

	if (curr_size < min_size) {
		return 0;
	} else {
		return curr_size - min_size;
	}
}

static bool RewriteAllLumps(struct progress_window *progress,
                            struct wad_file *f)
{
	VFILE *lump;
	uint32_t new_pos;
	int i;

	if (vfseek(f->vfs, f->write_pos, SEEK_SET) != 0) {
		return false;
	}

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

	f->write_pos = vftell(f->vfs);
	WriteDirectory(f);
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
	f->write_pos = sizeof(struct wad_file_header);
	if (!RewriteAllLumps(&progress, f)) {
		return false;
	}

	// Seek to new EOF, truncate, and we're done.
	new_eof = CURR_HEADER(f)->table_offset
	        + f->num_lumps * WAD_FILE_ENTRY_LEN;
	if (vfseek(f->vfs, new_eof, SEEK_SET) != 0) {
		return false;
	}
	vftruncate(f->vfs);

	// We cannot undo any more.
	f->revisions[0] = *CURR_REVISION(f);
	memmove(&f->revisions[0], CURR_REVISION(f),
	        sizeof(struct wad_revision));
	f->current_revision = 0;
	f->num_revisions = 1;
	CURR_REVISION(f)->eof = new_eof;
	return true;
}

int W_CanUndo(struct wad_file *wf)
{
	return wf->current_revision;
}

bool W_Undo(struct wad_file *wf, unsigned int levels)
{
	assert(levels <= W_CanUndo(wf));
	wf->current_revision -= levels;
	if (!ReadDirectory(wf)) {
		wf->current_revision += levels;
		return false;
	}
	WriteHeader(wf);
	wf->write_pos = CURR_REVISION(wf)->eof;
	wf->dirty = false;
	return true;
}

int W_CanRedo(struct wad_file *wf)
{
	return wf->num_revisions - wf->current_revision - 1;
}

bool W_Redo(struct wad_file *wf, unsigned int levels)
{
	assert(levels <= W_CanRedo(wf));
	wf->current_revision += levels;
	if (!ReadDirectory(wf)) {
		wf->current_revision -= levels;
		return false;
	}
	WriteHeader(wf);
	wf->write_pos = CURR_REVISION(wf)->eof;
	wf->dirty = false;
	return true;
}
