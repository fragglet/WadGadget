
int UI_ConfirmDialogBox(const char *title, const char *msg, ...);

void UI_MessageBox(const char *msg, ...);

// Returns string that was entered or NULL if cancelled. Caller owns string.
char *UI_TextInputDialogBox(char *title, size_t max_chars, char *msg, ...);

