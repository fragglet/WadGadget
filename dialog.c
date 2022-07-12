
#include <curses.h>

#include "colors.h"
#include "common.h"
#include "pane.h"
#include "ui.h"

struct confirm_dialog_box {
	struct pane pane;
	char *title;
	char msg[128];
	int result;
};

static int StringWidth(char *s)
{
	int max_width = 0, cur_width = 0;
	char *p;

	for (p = s; *p != '\0'; ++p) {
		if (*p != '\n') {
			++cur_width;
			continue;
		}
		max_width = max(max_width, cur_width);
		cur_width = 0;
	}
	return max(cur_width, max_width);
}

static int StringHeight(char *s)
{
	int lines = 1;
	char *p;

	for (p = s; *p != '\0'; ++p) {
		if (*p == '\n' && *(p+1) != '\0') {
			++lines;
		}
	}
	return lines;
}

// Print string at given position in window, with newlines wrapping to the
// next line at the same starting x position.
static void PrintMultilineString(WINDOW *win, int y, int x, char *s)
{
	char *p;

	wmove(win, y, x);
	for (p = s; *p != '\0'; ++p) {
		if (*p == '\n') {
			++y;
			wmove(win, y, x);
		} else {
			waddch(win, *p);
		}
	}
}

static void DrawConfirmDialog(void *pane)
{
	struct confirm_dialog_box *dialog = pane;
	WINDOW *win = dialog->pane.window;
	int w, h;

	getmaxyx(win, h, w);

	wbkgdset(win, COLOR_PAIR(PAIR_DIALOG_BOX));
	wattron(win, A_BOLD);
	werase(win);
	box(win, 0, 0);
	if (dialog->title != NULL) {
		mvwaddstr(win, 0, 2, " ");
		waddstr(win, dialog->title);
		waddstr(win, " ");
	}

	PrintMultilineString(win, 1, 2, dialog->msg);
	mvwaddstr(win, h - 2, 1, " ESC - Cancel ");
	mvwaddstr(win, h - 2, w - 14, " Y - Confirm ");
	wattroff(win, A_BOLD);
}

static void ConfirmDialogKeypress(void *dialog, int key)
{
	struct confirm_dialog_box *d = dialog;

	if (key == 27) {
		d->result = 0;
		UI_ExitMainLoop();
	}
	if (key == 'Y' || key == 'y') {
		d->result = 1;
		UI_ExitMainLoop();
	}
}

int UI_ConfirmDialogBox(char *title, char *msg, ...)
{
	struct confirm_dialog_box dialog;
	int scrh, scrw;
	int w, h;
	va_list args;
	getmaxyx(stdscr, scrh, scrw);

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	w = max(StringWidth(dialog.msg) + 4, 35);
	h = StringHeight(dialog.msg) + 4;
	dialog.pane.window = newwin(h, w, (scrh / 2) - h, (scrw - w) / 2);
	dialog.pane.draw = DrawConfirmDialog;
	dialog.pane.keypress = ConfirmDialogKeypress;
	dialog.title = title;

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	return dialog.result;
}

struct text_input_dialog_box {
	struct pane pane;
	char *title;
	char msg[128];
	int result;
	struct text_input_box input;
};

static void DrawTextInputDialog(void *pane)
{
	struct text_input_dialog_box *dialog = pane;
	WINDOW *win = dialog->pane.window;
	int w, h;

	getmaxyx(win, h, w);
	w = w;

	wbkgdset(win, COLOR_PAIR(PAIR_DIALOG_BOX));
	wattron(win, A_BOLD);
	werase(win);
	wattron(win, A_BOLD);
	box(win, 0, 0);
	if (dialog->title != NULL) {
		mvwaddstr(win, 0, 2, " ");
		waddstr(win, dialog->title);
		waddstr(win, " ");
	}

	PrintMultilineString(win, 1, 2, dialog->msg);

	mvwaddstr(win, h - 2, 1, " ESC - Cancel ");
	mvwaddstr(win, h - 2, w - 16, " ENT - Confirm ");

	wattroff(win, A_BOLD);
	UI_TextInputDraw(&dialog->input);
}

static void TextInputDialogKeypress(void *dialog, int key)
{
	struct text_input_dialog_box *d = dialog;

	if (key == 27) {
		d->result = 0;
		UI_ExitMainLoop();
		return;
	}
	UI_TextInputKeypress(&d->input, key);
}

int UI_TextInputDialogBox(char *title, size_t max_chars, char *msg, ...)
{
	struct text_input_dialog_box dialog;
	int scrh, scrw;
	int w, h;
	va_list args;
	getmaxyx(stdscr, scrh, scrw);

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	w = max(StringWidth(dialog.msg) + 4, 35);
	h = StringHeight(dialog.msg) + 5;
	dialog.pane.window = newwin(h, w, (scrh / 2) - h, (scrw - w) / 2);
	dialog.pane.draw = DrawTextInputDialog;
	dialog.pane.keypress = TextInputDialogKeypress;
	dialog.title = title;
	dialog.result = 0;
	UI_TextInputInit(&dialog.input, dialog.pane.window, h - 4, max_chars);

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	return dialog.result;
}

