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

#include "common.h"
#include "fs/vfile.h"
#include "pager/pager.h"
#include "pager/hexdump.h"
#include "pager/plaintext.h"

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

static void SwitchToASCII(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;
	struct plaintext_pager_config *ptc = cfg->plaintext_config;

	if (ptc == NULL) {
		VFILE *in = vfopenmem(cfg->data, cfg->data_len);
		ptc = checked_calloc(1, sizeof(struct plaintext_pager_config));
		assert(P_InitPlaintextConfig(cfg->pc.title, ptc, in));
		cfg->plaintext_config = ptc;
		ptc->hexdump_config = cfg;
	}

	P_SwitchConfig(&ptc->pc);
}

const struct action switch_ascii_action = {
	KEY_F(4), 0, "ASCII", "View as ASCII", SwitchToASCII,
};

static const struct action *hexdump_pager_actions[] = {
	&switch_ascii_action,
	NULL,
};

bool P_InitHexdumpConfig(const char *title, struct hexdump_pager_config *cfg,
                         VFILE *input)
{
	cfg->pc.title = title;
	cfg->pc.draw_line = DrawHexdumpLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = hexdump_pager_actions;
	cfg->plaintext_config = NULL;

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
	if (cfg.plaintext_config != NULL) {
		P_FreePlaintextConfig(cfg.plaintext_config);
	}
	P_FreeHexdumpConfig(&cfg);

	return true;
}
