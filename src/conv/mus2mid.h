//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2024 Simon Howard
// Copyright(C) 2006 Ben Ryves 2006
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// mus2mid.h - Ben Ryves 2006 - http://benryves.com - benryves@benryves.com
// Use to convert a MUS file into a single track, type 0 MIDI file.

#ifndef CONV__MUS2MID_H_INCLUDED
#define CONV__MUS2MID_H_INCLUDED

#include <stdbool.h>

#include "fs/vfile.h"

bool mus2mid(VFILE *musinput, VFILE *midioutput);

#endif /* #ifndef CONV__MUS2MID_H_INCLUDED */
