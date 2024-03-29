
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "colors.h"
#include "common.h"
#include "text_input.h"

void UI_TextInputInit(struct text_input_box *input, WINDOW *win, int y,
                      size_t max_chars)
{
	input->win = win;
	input->y = y;
	input->input_sz = max_chars + 1;
	input->input = checked_calloc(input->input_sz, 1);
	UI_TextInputClear(input);
}

void UI_TextInputDraw(struct text_input_box *input)
{
	char *s;
	size_t s_len;
	int w, h;
	int x;

	getmaxyx(input->win, h, w);
	w -= 4; h = h;

	wattron(input->win, COLOR_PAIR(PAIR_WHITE_BLACK));
	wmove(input->win, input->y, 2);
	for (x = 0; x < w; x++) {
		waddch(input->win, ' ');
	}

	wmove(input->win, input->y, 2);
	s_len = strlen(input->input);
	s = input->input;
	if (s_len + 1 > w) {
		s += s_len + 1 - w;
	}
	waddstr(input->win, s);
	wattroff(input->win, COLOR_PAIR(PAIR_WHITE_BLACK));
}

int UI_TextInputKeypress(struct text_input_box *input, int keypress)
{
	size_t pos;

	if (keypress == KEY_BACKSPACE && strlen(input->input) > 0) {
		input->input[strlen(input->input) - 1] = '\0';
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

