//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__NETPBM_H_IMPORTED
#define CONV__NETPBM_H_IMPORTED

#include <stdbool.h>

#include "fs/vfile.h"

bool NetpbmFileTypeSupported(const char *filename);
VFILE *NetpbmConvertToPNG(VFILE *input, const char *filename);

#endif /* #ifndef CONV__NETPBM_H_IMPORTED */
