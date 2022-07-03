#include <curses.h>

struct list_pane {
	WINDOW *pane;
	unsigned int window_offset, selected;
	const char *title;
	const char *(*get_entry_str)(struct list_pane *p, unsigned int i);
};

void UI_DrawListPane(struct list_pane *pane);

