//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef HELP_TEXT_H_INCLUDED
#define HELP_TEXT_H_INCLUDED

struct help_file {
	const char *filename;
	const char *contents;
};

extern const struct help_file help_files[];

#endif /* #ifndef HELP_TEXT_H_INCLUDED */
