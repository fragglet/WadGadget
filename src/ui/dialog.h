//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef UI__DIALOG_H_INCLUDED
#define UI__DIALOG_H_INCLUDED

#include <time.h>
#include "common.h"

struct progress_window {
	const char *operation;
	int count, total;
	time_t last_update;
};

void UI_InitProgressWindow(struct progress_window *win, int total,
                           const char *operation);
void UI_UpdateProgressWindow(struct progress_window *win, const char *ctx);

void UI_ShowNonblockingWindow(const char *msg, ...) PRINTF_ATTRIBUTE(1, 2);
int UI_ConfirmDialogBox(const char *title, const char *yes, const char *no,
                        const char *msg, ...) PRINTF_ATTRIBUTE(4, 5);

void UI_MessageBox(const char *msg, ...) PRINTF_ATTRIBUTE(1, 2);

// Returns string that was entered or NULL if cancelled. Caller owns string.
char *UI_TextInputDialogBox(char *title, const char *action, size_t max_chars,
                            const char *msg, ...) PRINTF_ATTRIBUTE(4, 5);

#endif /* #ifndef UI__DIALOG_H_INCLUDED */
