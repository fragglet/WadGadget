//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

struct hexdump_pager_config {
	struct pager_config pc;
	uint8_t *data;
	size_t data_len;
};

bool P_InitHexdumpConfig(const char *title, struct hexdump_pager_config *cfg,
                         VFILE *input);
void P_FreeHexdumpConfig(struct hexdump_pager_config *cfg);
bool P_RunHexdumpPager(const char *title, VFILE *input);
