//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>

struct text_input_box {
	WINDOW *win, *parent_win;
	char *input;
	size_t input_sz;
};

void UI_TextInputInit(struct text_input_box *input, WINDOW *win,
                      size_t max_chars);
void UI_TextInputDraw(struct text_input_box *input);
int UI_TextInputKeypress(struct text_input_box *input, int keypress);
void UI_TextInputClear(struct text_input_box *input);

