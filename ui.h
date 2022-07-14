
#include "pane.h"
#include "text_input.h"

#define FILE_PANE_WIDTH  27
#define FILE_PANE_HEIGHT 24

void UI_InitHeaderPane(struct pane *pane, WINDOW *win);

int UI_StringWidth(char *s);
int UI_StringHeight(char *s);
void UI_PrintMultilineString(WINDOW *win, int y, int x, const char *s);

