//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include "ui/pane.h"

typedef void (*pager_draw_line_fn)(WINDOW *win, unsigned int line,
                                   void *user_data);

struct pager_config {
	const char *title;
	pager_draw_line_fn draw_line;
	void *user_data;
	unsigned int num_lines;
};

struct pager {
	struct pane pane;
	WINDOW *line_win;
	bool done;
	unsigned int window_offset;
	struct pager_config *cfg;
};

void P_InitPager(struct pager *p, struct pager_config *cfg);
void P_FreePager(struct pager *p);
void P_BlockOnInput(struct pager *p);
void P_RunPager(struct pager_config *cfg);
