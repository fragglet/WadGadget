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
#include "pager/plaintext.h"

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

bool P_InitPlaintextConfig(const char *title,
                           struct plaintext_pager_config *cfg, VFILE *input)
{
	char *data, *p;
	size_t data_len, remaining;
	unsigned int lineno;

	cfg->pc.title = title;
	cfg->pc.draw_line = DrawPlaintextLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = NULL;

	data = vfreadall(input, &data_len);
	vfclose(input);
	if (data == NULL) {
		return false;
	}

	p = data;
	remaining = data_len;
	cfg->pc.num_lines = 1;
	for (;;) {
		char *newline = memchr(p, '\n', remaining);
		if (newline == NULL) {
			break;
		}
		++cfg->pc.num_lines;
		remaining -= newline - p + 1;
		p = newline + 1;
	}

	cfg->lines = checked_calloc(cfg->pc.num_lines, sizeof(char *));
	p = data;
	remaining = data_len;
	lineno = 0;

	while (lineno < cfg->pc.num_lines) {
		char *newline = memchr(p, '\n', remaining);
		size_t line_bytes;

		if (newline != NULL) {
			line_bytes = newline - p;
		} else {
			line_bytes = remaining;
		}

		cfg->lines[lineno] = checked_calloc(line_bytes + 1, 1);
		memcpy(cfg->lines[lineno], p, line_bytes);

		++lineno;

		if (newline != NULL) {
			++line_bytes;
		} else {
			break;
		}

		remaining -= line_bytes;
		p += line_bytes;
	}
	if (strlen(cfg->lines[cfg->pc.num_lines - 1]) == 0) {
		free(cfg->lines[cfg->pc.num_lines - 1]);
		--cfg->pc.num_lines;
	}

	return true;
}

bool P_RunPlaintextPager(const char *title, VFILE *input)
{
	struct plaintext_pager_config cfg;

	if (!P_InitPlaintextConfig(title, &cfg, input)) {
		return false;
	}

	P_RunPager(&cfg.pc);
	P_FreePlaintextConfig(&cfg);

	return true;
}
