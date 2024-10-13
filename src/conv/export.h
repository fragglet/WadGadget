//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__EXPORT_H_INCLUDED
#define CONV__EXPORT_H_INCLUDED

#include "lump_info.h"
#include "fs/vfs.h"

bool ExportToFile(struct directory *from, struct directory_entry *ent,
                  const struct lump_type *lt, const char *to_filename,
                  bool convert);
bool PerformExport(struct directory *from, struct file_set *from_set,
                   struct directory *to, struct file_set *result, bool convert);

#endif /* #ifndef CONV__EXPORT_H_INCLUDED */
