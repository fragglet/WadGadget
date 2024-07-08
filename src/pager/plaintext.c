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
#include <string.h>

#include "common.h"
#include "fs/vfile.h"
#include "pager/pager.h"
#include "pager/hexdump.h"
#include "pager/plaintext.h"
#include "ui/actions_bar.h"

static void SwitchToHexdump(void)
{
	struct plaintext_pager_config *cfg = current_pager->cfg->user_data;
	struct hexdump_pager_config *hdc = cfg->hexdump_config;

	if (hdc == NULL) {
		VFILE *in = vfopenmem(cfg->data, cfg->data_len);
		hdc = checked_calloc(1, sizeof(struct hexdump_pager_config));
		assert(P_InitHexdumpConfig(cfg->pc.title, hdc, in));
		cfg->hexdump_config = hdc;
		hdc->plaintext_config = cfg;
	}

	P_SwitchConfig(&hdc->pc);
}

static const struct action switch_hexdump_action = {
	0, 'D', "Hexdump", "View Hexdump", SwitchToHexdump,
};

static void ExitPagerAndEdit(void)
{
	struct plaintext_pager_config *cfg = current_pager->cfg->user_data;
	cfg->want_edit = true;
	UI_ExitMainLoop();
}

static const struct action exit_pager_edit_action = {
	0, 'E', "Edit", "Edit",  ExitPagerAndEdit,
};

static const struct action *plaintext_pager_actions[] = {
	&exit_pager_edit_action,
	&exit_pager_action,
	&switch_hexdump_action,
	&pager_search_action,
	&pager_search_again_action,
	NULL,
};

static void DrawPlaintextLine(WINDOW *win, unsigned int line, void *user_data)
{
	struct plaintext_pager_config *cfg = user_data;

	assert(line < cfg->pc.num_lines);
	waddstr(win, cfg->lines[line]);
}

void P_FreePlaintextConfig(struct plaintext_pager_config *cfg)
{
	int i;

	for (i = 0; i < cfg->pc.num_lines; i++) {
		free(cfg->lines[i]);
	}
	free(cfg->lines);
}

char **P_PlaintextLines(const char *data, size_t data_len, size_t *num_lines)
{
	char **lines;
	const char *p;
	size_t remaining;
	unsigned int lineno;

	p = data;
	remaining = data_len;
	*num_lines = 1;
	for (;;) {
		char *newline = memchr(p, '\n', remaining);
		if (newline == NULL) {
			break;
		}
		++*num_lines;
		remaining -= newline - p + 1;
		p = newline + 1;
	}

	lines = checked_calloc(*num_lines, sizeof(char *));
	p = data;
	remaining = data_len;
	lineno = 0;

	while (lineno < *num_lines) {
		char *newline = memchr(p, '\n', remaining);
		size_t line_bytes;

		if (newline != NULL) {
			line_bytes = newline - p;
		} else {
			line_bytes = remaining;
		}

		lines[lineno] = checked_calloc(line_bytes + 1, 1);
		memcpy(lines[lineno], p, line_bytes);

		++lineno;

		if (newline != NULL) {
			++line_bytes;
		} else {
			break;
		}

		remaining -= line_bytes;
		p += line_bytes;
	}

	return lines;
}

bool P_InitPlaintextConfig(const char *title, bool editable,
                           struct plaintext_pager_config *cfg, VFILE *input)
{
	cfg->pc.title = title;
	cfg->pc.draw_line = DrawPlaintextLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = editable ? plaintext_pager_actions
	                           : plaintext_pager_actions + 1;
	cfg->hexdump_config = NULL;
	cfg->pc.get_link = NULL;
	cfg->pc.current_link = -1;
	cfg->want_edit = false;

	cfg->data = vfreadall(input, &cfg->data_len);
	vfclose(input);
	if (cfg->data == NULL) {
		return false;
	}

	cfg->lines = P_PlaintextLines((char *) cfg->data, cfg->data_len,
	                              &cfg->pc.num_lines);
	cfg->pc.num_links = 0;

	return true;
}

enum plaintext_pager_result P_RunPlaintextPager(
	const char *title, VFILE *input, bool editable)
{
	struct pager p;
	struct plaintext_pager_config cfg;

	if (!P_InitPlaintextConfig(title, editable, &cfg, input)) {
		return PLAINTEXT_PAGER_FAILURE;
	}

	P_InitPager(&p, &cfg.pc);
	P_RunPager(&p);
	P_FreePager(&p);
	if (cfg.hexdump_config != NULL) {
		P_FreeHexdumpConfig(cfg.hexdump_config);
	}
	P_FreePlaintextConfig(&cfg);

	return cfg.want_edit ? PLAINTEXT_PAGER_WANT_EDIT
	                     : PLAINTEXT_PAGER_DONE;
}
