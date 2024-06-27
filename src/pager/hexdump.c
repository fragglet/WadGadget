//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>

#include "fs/vfile.h"
#include "pager/pager.h"
#include "pager/hexdump.h"

static void DrawHexdumpLine(WINDOW *win, unsigned int line, void *user_data)
{
	struct hexdump_pager_config *cfg = user_data;
	char buf[12];
	int i;

	snprintf(buf, sizeof(buf), " %08x: ", line * 16);
	waddstr(win, buf);

	for (i = 0; i < 16 && line * 16 + i < cfg->data_len; i++) {
		snprintf(buf, sizeof(buf), "%02x ", cfg->data[line * 16 + i]);
		waddstr(win, buf);
	}

	mvwaddstr(win, 0, 60, "");

	for (i = 0; i < 16 && line * 16 + i < cfg->data_len; i++) {
		int c = cfg->data[line * 16 + i];
		if (c >= 32 && c < 127) {
			waddch(win, c);
		} else {
			waddch(win, '.');
		}
	}
}

bool P_InitHexdumpConfig(const char *title, struct hexdump_pager_config *cfg,
                         VFILE *input)
{
	cfg->pc.title = title;
	cfg->pc.draw_line = DrawHexdumpLine;
	cfg->pc.user_data = cfg;

	cfg->data = vfreadall(input, &cfg->data_len);
	vfclose(input);
	cfg->pc.num_lines = (cfg->data_len + 15) / 16;

	return cfg->data != NULL;
}

void P_FreeHexdumpConfig(struct hexdump_pager_config *cfg)
{
	free(cfg->data);
}

bool P_RunHexdumpPager(const char *title, VFILE *input)
{
	struct hexdump_pager_config cfg;

	if (!P_InitHexdumpConfig(title, &cfg, input)) {
		return false;
	}

	P_RunPager(&cfg.pc);
	P_FreeHexdumpConfig(&cfg);

	return true;
}
