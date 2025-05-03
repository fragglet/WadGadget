//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef TEXTURES__INTERNAL_H_INCLUDED
#define TEXTURES__INTERNAL_H_INCLUDED

#include "textures/textures.h"

size_t TX_TextureLen(size_t patchcount);
struct pnames *TX_PnamesList(struct directory *_dir);

#endif /* #ifndef TEXTURES__INTERNAL_H_INCLUDED */
