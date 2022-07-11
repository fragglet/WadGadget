
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
void UI_TextInputKeypress(struct text_input_box *input, int keypress);

