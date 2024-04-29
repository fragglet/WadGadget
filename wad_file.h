//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef WAD_FILE_INCLUDED
#define WAD_FILE_INCLUDED

#include "vfile.h"

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
void W_CloseFile(struct wad_file *f);
struct wad_file_entry *W_GetDirectory(struct wad_file *f);
unsigned int W_NumLumps(struct wad_file *f);
VFILE *W_OpenLump(struct wad_file *f, unsigned int lump_index);
VFILE *W_OpenLumpRewrite(struct wad_file *f, unsigned int lump_index);
void W_WriteDirectory(struct wad_file *f);

// Insert new WAD entries before the lump at the given index. If
// `before_index == W_NumLumps()` then the new lumps are inserted at the
// end of the directory.
void W_AddEntries(struct wad_file *f, unsigned int before_index,
                  unsigned int count);
void W_DeleteEntry(struct wad_file *f, unsigned int index);
void W_SetLumpName(struct wad_file *f, unsigned int index, const char *name);
size_t W_ReadLumpHeader(struct wad_file *f, unsigned int index,
                        uint8_t *buf, size_t buf_len);

#endif /* #ifndef WAD_FILE_INCLUDED */

