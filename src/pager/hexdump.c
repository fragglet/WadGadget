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
#include "pager/help.h"
#include "pager/hexdump.h"
#include "pager/plaintext.h"
#include "ui/dialog.h"

static void DrawHexdumpLine(WINDOW *win, unsigned int line, void *user_data)
{
	struct hexdump_pager_config *cfg = user_data;
	char buf[12];
	int i;

	snprintf(buf, sizeof(buf), " %08x: ", line * cfg->columns);
	waddstr(win, buf);

	i = 0;
	while (i < cfg->columns && line * cfg->columns + i < cfg->data_len) {
		snprintf(buf, sizeof(buf), "%02x ",
		         cfg->data[line * cfg->columns + i]);
		waddstr(win, buf);
		++i;
	}

	mvwaddstr(win, 0, cfg->columns * 3 + 12, "");

	i = 0;
	while (i < cfg->columns && line * cfg->columns + i < cfg->data_len) {
		int c = cfg->data[line * cfg->columns + i];
		if (c >= 32 && c < 127) {
			waddch(win, c);
		} else {
			waddch(win, '.');
		}
		++i;
	}
}

static void SwitchToASCII(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;
	struct plaintext_pager_config *ptc = cfg->plaintext_config;

	if (ptc == NULL) {
		VFILE *in = vfopenmem(cfg->data, cfg->data_len);
		ptc = checked_calloc(1, sizeof(struct plaintext_pager_config));
		assert(P_InitPlaintextConfig(cfg->pc.title, false, ptc, in));
		cfg->plaintext_config = ptc;
		ptc->hexdump_config = cfg;
	}

	P_SwitchConfig(&ptc->pc);
}

const struct action switch_ascii_action = {
	0, 'D', "ASCII", "View as ASCII", SwitchToASCII,
};

static void ChangeColumns(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;
	char *answer;
	int cols;

	answer = UI_TextInputDialogBox(
		"Change columns per line", "Set columns", 2,
		"Enter the number of columns\nto display per line:");
	if (answer == NULL) {
		return;
	}

	cols = atoi(answer);
	if (cols <= 0) {
		return;
	}

	current_pager->window_offset =
		current_pager->window_offset * cfg->columns / cols;

	cfg->columns = cols;
	cfg->pc.num_lines = (cfg->data_len + cols - 1) / cols;
}

const struct action change_columns_action = {
	0, 'O', "Columns", "Columns", ChangeColumns,
};

static void OpenDoomSpecs(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;

	if (cfg->specs_help.pc.title == NULL) {
		if (!P_InitHelpConfig(&cfg->specs_help, "uds.md")) {
			cfg->specs_help.pc.title = NULL;
			return;
		}
		P_InitPager(&cfg->specs_pager, &cfg->specs_help.pc);
	}
	P_RunPager(&cfg->specs_pager);
}

const struct action open_specs_action = {
	0, 'U', "Specs", "Open Doom Specs", OpenDoomSpecs,
};

static const struct action *hexdump_pager_actions[] = {
	&exit_pager_action,
	&switch_ascii_action,
	&change_columns_action,
	&pager_search_action,
	&pager_search_again_action,
	&open_specs_action,
	NULL,
};

bool P_InitHexdumpConfig(const char *title, struct hexdump_pager_config *cfg,
                         VFILE *input)
{
	cfg->pc.title = title;
	cfg->pc.draw_line = DrawHexdumpLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = hexdump_pager_actions;
	cfg->pc.get_link = NULL;
	cfg->plaintext_config = NULL;
	cfg->specs_help.pc.title = NULL;

	cfg->data = vfreadall(input, &cfg->data_len);
	vfclose(input);
	cfg->columns = 16;
	cfg->pc.num_lines = (cfg->data_len + cfg->columns - 1) / cfg->columns;
	cfg->pc.num_links = 0;
	cfg->pc.current_link = -1;
	cfg->pc.current_column = 0;

	return cfg->data != NULL;
}

void P_FreeHexdumpConfig(struct hexdump_pager_config *cfg)
{
	free(cfg->data);
	if (cfg->specs_help.pc.title != NULL) {
		P_FreeHelpConfig(&cfg->specs_help);
		P_FreePager(&cfg->specs_pager);
	}
}

bool P_RunHexdumpPager(const char *title, VFILE *input)
{
	struct pager p;
	struct hexdump_pager_config cfg;

	if (!P_InitHexdumpConfig(title, &cfg, input)) {
		return false;
	}

	P_InitPager(&p, &cfg.pc);
	P_RunPager(&p);
	P_FreePager(&p);
	if (cfg.plaintext_config != NULL) {
		P_FreePlaintextConfig(cfg.plaintext_config);
	}
	P_FreeHexdumpConfig(&cfg);

	return true;
}
