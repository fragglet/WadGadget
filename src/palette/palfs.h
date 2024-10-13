//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef PALETTE__PALFS_H_INCLUDED
#define PALETTE__PALFS_H_INCLUDED

struct directory *PAL_OpenDirectory(struct directory *previous);
struct directory *PAL_InnerDir(struct directory *dir);
struct directory_entry *PAL_InnerEntry(struct directory *dir,
                                       struct directory_entry *ent);

#endif /* #ifndef PALETTE__PALFS_H_INCLUDED */
