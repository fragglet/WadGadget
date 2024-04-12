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

int UI_ConfirmDialogBox(const char *title, const char *msg, ...);

void UI_MessageBox(const char *msg, ...);

// Returns string that was entered or NULL if cancelled. Caller owns string.
char *UI_TextInputDialogBox(char *title, size_t max_chars, char *msg, ...);

