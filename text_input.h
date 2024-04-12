//
// Copyright(C) 2022-2024 Simon Howard
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>

struct text_input_box {
	WINDOW *win;
	int y;
	char *input;
	size_t input_sz;
};

void UI_TextInputInit(struct text_input_box *input, WINDOW *win, int y,
                      size_t max_chars);
void UI_TextInputDraw(struct text_input_box *input);
int UI_TextInputKeypress(struct text_input_box *input, int keypress);
void UI_TextInputClear(struct text_input_box *input);

