//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

int UI_ConfirmDialogBox(const char *title, const char *msg, ...);

void UI_MessageBox(const char *msg, ...);

// Returns string that was entered or NULL if cancelled. Caller owns string.
char *UI_TextInputDialogBox(char *title, size_t max_chars, char *msg, ...);

