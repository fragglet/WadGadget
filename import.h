//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "vfs.h"

bool ImportFromFile(VFILE *fromfile, const char *src_name,
                    struct wad_file *to_wad, int lumpnum,
                    bool flats_section, bool convert);
bool PerformImport(struct directory *from, struct file_set *from_set,
                   struct directory *to, int to_index,
                   struct file_set *result, bool convert);

