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
#include "ui/actions_bar.h"
#include "ui/pane.h"

typedef void (*pager_draw_line_fn)(WINDOW *win, unsigned int line,
                                   void *user_data);

struct pager_config {
	const char *title;
	pager_draw_line_fn draw_line;
	void *user_data;
	size_t num_lines;
	const struct action **actions;
};

struct pager {
	struct pane pane;
	WINDOW *search_pad;
	WINDOW *line_win;
	unsigned int window_offset;
	int search_line;
	struct pager_config *cfg;
	char subtitle[15];
};

void P_InitPager(struct pager *p, struct pager_config *cfg);
void P_FreePager(struct pager *p);
void P_BlockOnInput(struct pager *p);
void P_RunPager(struct pager_config *cfg);
void P_SwitchConfig(struct pager_config *cfg);
void P_JumpToLine(struct pager *p, int lineno);

extern struct pager *current_pager;
extern const struct action exit_pager_action;
extern const struct action pager_search_action;
