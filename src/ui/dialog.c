//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "ui/dialog.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <curses.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "ui/colors.h"
#include "common.h"
#include "ui/pane.h"
#include "ui/stack.h"
#include "ui/ui.h"
#include "ui/text_input.h"

struct nonblocking_window {
	struct pane pane;
	char msg[128];
};

static WINDOW *CenteredWindow(int w, int h)
{
	int top_line, num_lines;

	UI_GetDesktopLines(&top_line, &num_lines);

	return newwin(min(h, num_lines - 1), min(w, COLS),
	              top_line + max(1, (num_lines - h - 1) / 2),
	              max(0, (COLS - w - 1) / 2));
}

static bool DrawNonblockingWindow(void *pane)
{
	struct nonblocking_window *nbw = pane;
	WINDOW *win = nbw->pane.window;
	int w, h;

	wbkgdset(win, COLOR_PAIR(PAIR_DIALOG_BOX));
	werase(win);
	box(win, 0, 0);
	UI_DrawDropShadow(win);

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
	nbw.pane.mouse_click = NULL;

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
	int x, y;
	int key;
	int result;
};

struct confirm_dialog_box {
	struct pane pane;
	const char *title;
	struct dialog_button left, right;
	char msg[256];
	int result;
};

static int DialogButtonWidth(struct dialog_button *b)
{
	if (b->label == NULL) {
		return 0;
	}
	return 5 + strlen(b->label) + strlen(b->key_label);
}

static void DrawDialogButton(WINDOW *win, struct dialog_button *b)
{
	if (b->label == NULL) {
		return;
	}
	mvwaddstr(win, b->y, b->x, " ");
	waddstr(win, b->key_label);
	waddstr(win, " - ");
	waddstr(win, b->label);
	waddstr(win, " ");
}

static bool CheckButtonPress(struct dialog_button *b, int key, int *result)
{
	if (b->key != 0 && toupper(key) == toupper(b->key)) {
		*result = b->result;
		UI_ExitMainLoop();
		return true;
	}

	return false;
}

static void CheckButtonClick(struct dialog_button *b, int x, int y,
                             int *result)
{
	if (y == b->y && x >= b->x && x < b->x + DialogButtonWidth(b)) {
		*result = b->result;
		UI_ExitMainLoop();
	}
}

static bool DrawConfirmDialog(void *pane)
{
	struct confirm_dialog_box *dialog = pane;
	WINDOW *win = dialog->pane.window;

	wbkgdset(win, COLOR_PAIR(PAIR_DIALOG_BOX));
	wattron(win, A_BOLD);
	werase(win);
	box(win, 0, 0);
	UI_DrawDropShadow(win);

	if (dialog->title != NULL) {
		mvwaddstr(win, 0, 2, " ");
		waddstr(win, dialog->title);
		waddstr(win, " ");
	}

	UI_PrintMultilineString(win, 1, 2, dialog->msg);
	DrawDialogButton(win, &dialog->left);
	DrawDialogButton(win, &dialog->right);
	wattroff(win, A_BOLD);

	return true;
}

static void ConfirmDialogKeypress(void *dialog, int key)
{
	struct confirm_dialog_box *d = dialog;
	CheckButtonPress(&d->left, key, &d->result);
	CheckButtonPress(&d->right, key, &d->result);
}

static void ConfirmDialogMouseClick(void *dialog, int x, int y)
{
	struct confirm_dialog_box *d = dialog;
	CheckButtonClick(&d->left, x, y, &d->result);
	CheckButtonClick(&d->right, x, y, &d->result);
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
	dialog->pane.mouse_click = ConfirmDialogMouseClick;
	dialog->title = title;
	dialog->left.label = NULL;
	dialog->left.x = 1;
	dialog->left.y = h - 2;
	dialog->left.result = 0;
	dialog->right.label = NULL;
	dialog->right.x = w - 1;
	dialog->right.y = h - 2;
	dialog->right.result = 1;
}

int UI_ConfirmDialogBox(const char *title, const char *yes,
                        const char *no, const char *msg, ...)
{
	const struct action **saved_actions = UI_ActionsBarSetActions(NULL);
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
	dialog.right.x -= DialogButtonWidth(&dialog.right);

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	UI_ActionsBarSetActions(saved_actions);

	return dialog.result;
}

void UI_MessageBox(const char *msg, ...)
{
	const struct action **saved_actions = UI_ActionsBarSetActions(NULL);
	struct confirm_dialog_box dialog;
	va_list args;

	va_start(args, msg);
	vsnprintf(dialog.msg, sizeof(dialog.msg), msg, args);
	va_end(args);

	InitDialogBox(&dialog, NULL, msg);
	dialog.right.label = "Close";
	snprintf(dialog.right.key_label, sizeof(dialog.right.key_label), "Esc");
	dialog.right.key = 27;
	dialog.right.x -= DialogButtonWidth(&dialog.right);

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	UI_ActionsBarSetActions(saved_actions);
}

struct text_input_dialog_box {
	struct pane pane;
	char *title;
	char msg[128];
	int result;
	struct dialog_button left, right;
	struct text_input_box input;
};

static bool DrawTextInputDialog(void *pane)
{
	struct text_input_dialog_box *dialog = pane;
	WINDOW *win = dialog->pane.window;

	wbkgdset(win, COLOR_PAIR(PAIR_DIALOG_BOX));
	wattron(win, A_BOLD);
	werase(win);
	wattron(win, A_BOLD);
	box(win, 0, 0);
	UI_DrawDropShadow(win);

	if (dialog->title != NULL) {
		mvwaddstr(win, 0, 2, " ");
		waddstr(win, dialog->title);
		waddstr(win, " ");
	}

	UI_PrintMultilineString(win, 1, 2, dialog->msg);

	DrawDialogButton(win, &dialog->left);
	DrawDialogButton(win, &dialog->right);

	wattroff(win, A_BOLD);
	UI_TextInputDraw(&dialog->input);

	return true;
}

static void TextInputDialogKeypress(void *dialog, int key)
{
	struct text_input_dialog_box *d = dialog;

	if (!CheckButtonPress(&d->left, key, &d->result)
	 && !CheckButtonPress(&d->right, key, &d->result)) {
		UI_TextInputKeypress(&d->input, key);
	}
}

static void TextInputDialogMouseClick(void *dialog, int x, int y)
{
	struct text_input_dialog_box *d = dialog;

	CheckButtonClick(&d->left, x, y, &d->result);
	CheckButtonClick(&d->right, x, y, &d->result);
}

char *UI_TextInputDialogBox(char *title, const char *action, size_t max_chars,
                            char *msg, ...)
{
	const struct action **saved_actions = UI_ActionsBarSetActions(NULL);
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
	dialog.pane.mouse_click = TextInputDialogMouseClick;
	dialog.title = title;
	dialog.result = 0;

	snprintf(dialog.left.key_label, 8, "Esc");
	dialog.left.key = 27;
	dialog.left.label = "Cancel";
	dialog.left.x = 1;
	dialog.left.y = h - 2;
	dialog.left.result = 0;

	snprintf(dialog.right.key_label, 8, "Ent");
	dialog.right.key = '\r';
	dialog.right.label = action;
	dialog.right.x = w - DialogButtonWidth(&dialog.right) - 1;
	dialog.right.y = h - 2;
	dialog.right.result = 1;

	UI_TextInputInit(&dialog.input, dialog.pane.window, max_chars);
	mvderwin(dialog.input.win, h - 4, 2);
	wresize(dialog.input.win, 1, w - 4);

	UI_PaneShow(&dialog);
	UI_RunMainLoop();
	UI_PaneHide(&dialog);

	UI_ActionsBarSetActions(saved_actions);

	if (!dialog.result) {
		free(dialog.input.input);
		return NULL;
	}

	return dialog.input.input;
}
