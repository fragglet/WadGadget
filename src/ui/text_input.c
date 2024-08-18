//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ui/colors.h"
#include "common.h"
#include "ui/text_input.h"

void UI_TextInputInit(struct text_input_box *input, WINDOW *win,
                      size_t max_chars)
{
	input->win = derwin(win, 1, 10, 0, 0);
	input->parent_win = win;
	input->input_sz = max_chars + 1;
	input->input = checked_calloc(input->input_sz, 1);
	UI_TextInputClear(input);
}

void UI_TextInputDraw(struct text_input_box *input)
{
	char *s;
	size_t s_len;
	int w, x;

	w = getmaxx(input->win);

	wattron(input->win, COLOR_PAIR(PAIR_WHITE_BLACK));
	wmove(input->win, 0, 0);
	for (x = 0; x < w; x++) {
		waddch(input->win, ' ');
	}

	wmove(input->win, 0, 0);
	s_len = strlen(input->input);
	s = input->input;
	if (s_len + 1 > w) {
		s += s_len + 1 - w;
	}
	waddstr(input->win, s);
	wattroff(input->win, COLOR_PAIR(PAIR_WHITE_BLACK));

	// Cursor needs to be set via parent window
	mvwaddstr(input->parent_win,
	          getcury(input->win) + getpary(input->win),
	          getcurx(input->win) + getparx(input->win), "");
}

int UI_TextInputKeypress(struct text_input_box *input, int keypress)
{
	size_t pos;

	if ((keypress == KEY_BACKSPACE || keypress == 0x7f)
	 && strlen(input->input) > 0) {
		input->input[strlen(input->input) - 1] = '\0';
		return 1;
	}
	if (keypress == CTRL_('W')) {
		UI_TextInputClear(input);
		return 1;
	}

	if (keypress >= 128 || !isprint(keypress)) {
		return 0;
	}

	pos = strlen(input->input);
	if (pos + 1 >= input->input_sz) {
		return 0;
	}
	input->input[pos] = keypress;
	input->input[pos + 1] = '\0';
	return 1;
}

void UI_TextInputClear(struct text_input_box *input)
{
	input->input[0] = '\0';
}

