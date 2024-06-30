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
#include <string.h>
#include <ctype.h>

#include "ui/colors.h"
#include "common.h"
#include "ui/dialog.h"
#include "ui/pane.h"
#include "ui/ui.h"

struct nonblocking_window {
	struct pane pane;
	char msg[128];
};

static WINDOW *CenteredWindow(int w, int h)
{
	int scrh, scrw;

	getmaxyx(stdscr, scrh, scrw);

	return newwin(min(h, scrh - 1), min(w, scrw),
	              max(1, (scrh - h - 1) / 2),
	              max(0, (scrw - w - 1) / 2));
}

static bool DrawNonblockingWindow(void *pane)
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

	return true;
}

void UI_ShowNonblockingWindow(const char *msg, ...)
{
	struct nonblocking_window nbw;
	va_list args;
	int w, h;

	va_start(args, msg);
	vsnprintf(nbw.msg, sizeof(nbw.msg), msg, args);
	va_end(args);

	w = max(UI_StringWidth(nbw.msg) + 4, 35);
	h = UI_StringHeight(nbw.msg) + 2;

	nbw.pane.window = CenteredWindow(w, h);
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

struct dialog_button {
	char key_label[8];
	const char *label;
	int key;
};

struct confirm_dialog_box {
	struct pane pane;
	const char *title;
	struct dialog_button left, right;
	char msg[256];
	int result;
};

static bool DrawConfirmDialog(void *pane)
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
	if (dialog->left.label != NULL) {
		mvwaddstr(win, h - 2, 1, " ");
		waddstr(win, dialog->left.key_label);
		waddstr(win, " - ");
		waddstr(win, dialog->left.label);
		waddstr(win, " ");
	}
	if (dialog->right.label != NULL) {
		int x = w - strlen(dialog->right.label)
		      - strlen(dialog->right.key_label) - 6;
		mvwaddstr(win, h - 2, x, " ");
		waddstr(win, dialog->right.key_label);
		waddstr(win, " - ");
		waddstr(win, dialog->right.label);
		waddstr(win, " ");
	}
	wattroff(win, A_BOLD);

	return true;
}

static void ConfirmDialogKeypress(void *dialog, int key)
{
	struct confirm_dialog_box *d = dialog;

	if (d->left.key != 0 && toupper(key) == toupper(d->left.key)) {
		d->result = 0;
		UI_ExitMainLoop();
	}
	if (d->right.key != 0 && toupper(key) == toupper(d->right.key)) {
		d->result = 1;
		UI_ExitMainLoop();
	}
}

static void InitDialogBox(struct confirm_dialog_box *dialog,
                          const char *title, const char *msg)
{
	int w, h;

	w = max(UI_StringWidth(dialog->msg) + 4, 35);
	h = UI_StringHeight(dialog->msg) + 4;
	dialog->pane.window = CenteredWindow(w, h);
	dialog->pane.draw = DrawConfirmDialog;
	dialog->pane.keypress = ConfirmDialogKeypress;
	dialog->title = title;
	dialog->left.label = NULL;
	dialog->right.label = NULL;
}

int UI_ConfirmDialogBox(const char *title, const char *yes,
                        const char *no, const char *msg, ...)
{
	struct confirm_dialog_box dialog;
	va_list args;

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	InitDialogBox(&dialog, title, msg);

	dialog.left.label = no;
	snprintf(dialog.left.key_label, sizeof(dialog.left.key_label), "Esc");
	dialog.left.key = 27;

	dialog.right.label = yes;
	snprintf(dialog.right.key_label, sizeof(dialog.right.key_label), "Y");
	dialog.right.key = 'Y';

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
	dialog.right.label = "Close";
	snprintf(dialog.right.key_label, sizeof(dialog.right.key_label), "Esc");
	dialog.right.key = 27;

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);
}

struct text_input_dialog_box {
	struct pane pane;
	char *title;
	const char *action;
	char msg[128];
	int result;
	struct text_input_box input;
};

static bool DrawTextInputDialog(void *pane)
{
	struct text_input_dialog_box *dialog = pane;
	WINDOW *win = dialog->pane.window;
	int w, h;

	getmaxyx(win, h, w);

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

	mvwaddstr(win, h - 2, 1, " Esc - Cancel ");
	mvwaddstr(win, h - 2, w - strlen(dialog->action) - 9, " Ent - ");
	waddstr(win, dialog->action);
	waddstr(win, " ");

	wattroff(win, A_BOLD);
	UI_TextInputDraw(&dialog->input);

	return true;
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

char *UI_TextInputDialogBox(char *title, const char *action, size_t max_chars,
                            char *msg, ...)
{
	struct text_input_dialog_box dialog;
	int w, h;
	va_list args;

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	w = max(UI_StringWidth(dialog.msg) + 4, 35);
	h = UI_StringHeight(dialog.msg) + 5;
	dialog.pane.window = CenteredWindow(w, h);
	dialog.pane.draw = DrawTextInputDialog;
	dialog.pane.keypress = TextInputDialogKeypress;
	dialog.title = title;
	dialog.action = action;
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

