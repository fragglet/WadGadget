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
#include <stdbool.h>
#include <stdarg.h>
#include <curses.h>

#include "colors.h"
#include "common.h"
#include "dialog.h"
#include "pane.h"
#include "ui.h"

struct nonblocking_window {
	struct pane pane;
	char msg[128];
};

static void DrawNonblockingWindow(void *pane)
{
	struct nonblocking_window *nbw = pane;
	WINDOW *win = nbw->pane.window;
	int w, h;

	wbkgdset(win, COLOR_PAIR(PAIR_DIALOG_BOX));
	werase(win);
	box(win, 0, 0);

	UI_PrintMultilineString(win, 1, 2, nbw->msg);
	wattroff(win, A_BOLD);

	getmaxyx(win, h, w);
	mvwaddstr(win, h - 1, w - 2, "");
}

void UI_ShowNonblockingWindow(const char *msg, ...)
{
	struct nonblocking_window nbw;
	va_list args;
	int scrh, scrw;
	int w, h;

	va_start(args, msg);
	vsnprintf(nbw.msg, sizeof(nbw.msg), msg, args);
	va_end(args);

	getmaxyx(stdscr, scrh, scrw);

	w = max(UI_StringWidth(nbw.msg) + 4, 35);
	h = UI_StringHeight(nbw.msg) + 2;

	nbw.pane.window = newwin(h, w, (scrh / 2) - h, (scrw - w) / 2);
	nbw.pane.draw = DrawNonblockingWindow;
	nbw.pane.keypress = NULL;

	UI_PaneShow(&nbw);
	UI_DrawAllPanes();
	UI_PaneHide(&nbw);
}

void UI_InitProgressWindow(struct progress_window *win, int total,
                           const char *operation)
{
	win->operation = operation;
	win->count = 0;
	win->total = total;
	win->last_update = 0;
}

void UI_UpdateProgressWindow(struct progress_window *win, const char *ctx)
{
	clock_t now = clock();

	++win->count;
	if (now - win->last_update > (CLOCKS_PER_SEC / 4)) {
		UI_ShowNonblockingWindow("%s (%d / %d)...\n%s",
		                         win->operation, win->count,
		                         win->total, ctx);
		win->last_update = now;
	}
}

struct confirm_dialog_box {
	struct pane pane;
	const char *title;
	char msg[128];
	int result;
	bool has_confirm;
};

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

	UI_PrintMultilineString(win, 1, 2, dialog->msg);
	if (dialog->has_confirm) {
		mvwaddstr(win, h - 2, 1, " ESC - Cancel ");
		mvwaddstr(win, h - 2, w - 14, " Y - Confirm ");
	} else {
		mvwaddstr(win, h - 2, w - 14, " ESC - Close ");
	}
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

static void InitDialogBox(struct confirm_dialog_box *dialog,
                          const char *title, const char *msg)
{
	int scrh, scrw;
	int w, h;
	getmaxyx(stdscr, scrh, scrw);

	w = max(UI_StringWidth(dialog->msg) + 4, 35);
	h = UI_StringHeight(dialog->msg) + 4;
	dialog->pane.window = newwin(h, w, (scrh / 2) - h, (scrw - w) / 2);
	dialog->pane.draw = DrawConfirmDialog;
	dialog->pane.keypress = ConfirmDialogKeypress;
	dialog->title = title;
}

int UI_ConfirmDialogBox(const char *title, const char *msg, ...)
{
	struct confirm_dialog_box dialog;
	va_list args;

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	InitDialogBox(&dialog, title, msg);
	dialog.has_confirm = true;

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	return dialog.result;
}

void UI_MessageBox(const char *msg, ...)
{
	struct confirm_dialog_box dialog;
	va_list args;

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	InitDialogBox(&dialog, NULL, msg);
	dialog.has_confirm = false;

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);
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

	UI_PrintMultilineString(win, 1, 2, dialog->msg);

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
	if (key == '\r') {
		d->result = 1;
		UI_ExitMainLoop();
	}
	UI_TextInputKeypress(&d->input, key);
}

char *UI_TextInputDialogBox(char *title, size_t max_chars, char *msg, ...)
{
	struct text_input_dialog_box dialog;
	int scrh, scrw;
	int w, h;
	va_list args;
	getmaxyx(stdscr, scrh, scrw);

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	w = max(UI_StringWidth(dialog.msg) + 4, 35);
	h = UI_StringHeight(dialog.msg) + 5;
	dialog.pane.window = newwin(h, w, (scrh / 2) - h, (scrw - w) / 2);
	dialog.pane.draw = DrawTextInputDialog;
	dialog.pane.keypress = TextInputDialogKeypress;
	dialog.title = title;
	dialog.result = 0;
	UI_TextInputInit(&dialog.input, dialog.pane.window, max_chars);
	mvderwin(dialog.input.win, h - 4, 2);
	wresize(dialog.input.win, 1, w - 4);

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	if (!dialog.result) {
		free(dialog.input.input);
		return NULL;
	}

	return dialog.input.input;
}

