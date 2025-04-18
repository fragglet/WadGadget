//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI__SEARCH_PANE_H_INCLUDED
#define UI__SEARCH_PANE_H_INCLUDED

#include <curses.h>
#include <stddef.h>

#include "ui/pane.h"
#include "ui/text_input.h"

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

void UI_InitSearchPane(struct search_pane *sp, WINDOW *win);

#endif /* #ifndef UI__SEARCH_PANE_H_INCLUDED */
