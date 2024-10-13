//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef VIEW_H_INCLUDED
#define VIEW_H_INCLUDED

#include "fs/vfs.h"

enum open_result { OPEN_FAILED, OPEN_VIEWED, OPEN_EDITED };

enum open_result OpenFile(const char *filename,
                          const struct directory_entry *ent,
                          bool force_edit);
void OpenDirent(struct directory *dir, struct directory_entry *ent,
                bool force_edit);
void RunShell(void);

#endif /* #ifndef VIEW_H_INCLUDED */
