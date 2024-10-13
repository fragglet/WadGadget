//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef PAGER__HEXDUMP_H_INCLUDED
#define PAGER__HEXDUMP_H_INCLUDED

#include "pager/help.h"

struct hexdump_pager_config {
	struct pager_config pc;
	uint8_t *data;
	size_t data_len;
	void *plaintext_config;
	int columns;
	int record_length;

	// If the user presses ^U, we bring up a help pager showing the
	// Unofficial Doom Specs; they might use the documentation from the
	// specs to interpret the bytes they're seeing. But if the user
	// closes this help pager, we want them to be able to open it up
	// again to exactly the same location, so they don't need to go
	// navigating and searching again to find what they were looking at.
	// If specs_help.pc.title == NULL, this is not initialized yet.
	struct pager specs_pager;
	struct help_pager_config specs_help;
	bool specs_pager_open;
};

bool P_InitHexdumpConfig(const char *title, struct hexdump_pager_config *cfg,
                         VFILE *input);
void P_FreeHexdumpConfig(struct hexdump_pager_config *cfg);
bool P_RunHexdumpPager(const char *title, VFILE *input);

#endif /* #ifndef PAGER__HEXDUMP_H_INCLUDED */
