//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__GFXEDIT_H_INCLUDED
#define CONV__GFXEDIT_H_INCLUDED

#include "fs/wad_file.h"
#include <stdint.h>

bool V_EditGraphic(struct wad_file *wf, unsigned int lump_index);

#endif /* #ifndef CONV__GFXEDIT_H_INCLUDED */
