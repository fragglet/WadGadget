//
// Copyright(C) 2026 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__ACS_H_INCLUDED
#define CONV__ACS_H_INCLUDED

#include "fs/vfile.h"

VFILE *ACS_Assemble(VFILE *in);
VFILE *ACS_Disassemble(VFILE *in);

#endif /* #ifndef CONV__ACS_H_INCLUDED */
