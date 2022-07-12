
int UI_ConfirmDialogBox(char *title, char *msg, ...);

// Returns string that was entered or NULL if cancelled. Caller owns string.
char *UI_TextInputDialogBox(char *title, size_t max_chars, char *msg, ...);

