//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2024 Simon Howard
// Copyright(C) 2006 Ben Ryves 2006
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// mus2mid.h - Ben Ryves 2006 - http://benryves.com - benryves@benryves.com
// Use to convert a MUS file into a single track, type 0 MIDI file.

#ifndef MUS2MID_H
#define MUS2MID_H

#include "vfile.h"

bool mus2mid(VFILE *musinput, VFILE *midioutput);

#endif /* #ifndef MUS2MID_H */

