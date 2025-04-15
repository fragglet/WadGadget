//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef TEXTURES__EDITOR_H_INCLUDED
#define TEXTURES__EDITOR_H_INCLUDED

#include "textures/textures.h"

struct texture_editor {
	struct texture **tx;
	struct pnames *pnames;
};

void TX_EditTexture(struct texture **tx, struct pnames *pnames);

#endif /* #ifndef TEXTURES__EDITOR_H_INCLUDED */
