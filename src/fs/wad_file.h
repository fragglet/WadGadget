//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef FS__WAD_FILE_H_INCLUDED
#define FS__WAD_FILE_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fs/vfile.h"

#define LUMP_HEADER_LEN 8

struct wad_file;

struct wad_file_header {
	char id[4];
	unsigned int num_lumps;
	unsigned int table_offset;
};

struct wad_file_entry {
	unsigned int position;
	unsigned int size;
	char name[8];
	uint64_t serial_no;
	uint8_t lump_header[LUMP_HEADER_LEN];
};

bool W_CreateFile(const char *filename);
struct wad_file *W_OpenFile(const char *filename);
bool W_IsIWAD(struct wad_file *f);
bool W_IsReadOnly(struct wad_file *f);
void W_CloseFile(struct wad_file *f);
struct wad_file_entry *W_GetDirectory(struct wad_file *f);
int W_GetNumForName(struct wad_file *f, const char *name);
unsigned int W_NumLumps(struct wad_file *f);
VFILE *W_OpenLump(struct wad_file *f, unsigned int lump_index);
VFILE *W_OpenLumpRewrite(struct wad_file *f, unsigned int lump_index);

// Insert new WAD entries before the lump at the given index. If
// `before_index == W_NumLumps()` then the new lumps are inserted at the
// end of the directory.
void W_AddEntries(struct wad_file *f, unsigned int before_index,
                  unsigned int count);
void W_DeleteEntries(struct wad_file *f, unsigned int index, unsigned int cnt);
void W_DeleteEntry(struct wad_file *f, unsigned int index);
void W_SetLumpName(struct wad_file *f, unsigned int index, const char *name);
size_t W_ReadLumpHeader(struct wad_file *f, unsigned int index,
                        uint8_t *buf, size_t buf_len);
uint32_t W_NumJunkBytes(struct wad_file *f);
void W_SwapEntries(struct wad_file *f, unsigned int l1, unsigned int l2);

// Must be called after any change to the file by above functions
// (W_AddEntries, W_OpenLumpRewrite, etc.), otherwise the directory will
// not be updated and the changes will be lost.
bool W_NeedCommit(struct wad_file *f);
void W_CommitChanges(struct wad_file *f);

// Functions below this point take effect immediately and do not require
// calling W_CommitChanges().

bool W_CompactWAD(struct wad_file *f);

// Snapshotting functions for implementing undo/redo.
VFILE *W_SaveSnapshot(struct wad_file *wf);
void W_RestoreSnapshot(struct wad_file *wf, VFILE *in);

#endif /* #ifndef FS__WAD_FILE_H_INCLUDED */
