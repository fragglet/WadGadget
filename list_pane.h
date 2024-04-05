#ifndef INCLUDE_LIST_PANE_H
#define INCLUDE_LIST_PANE_H

#include "pane.h"

struct list_pane_funcs {
	void (*draw_element)(WINDOW *w, int index, void *data);
	unsigned int (*num_entries)(void *data);
};

struct list_pane {
	struct pane pane;
	const struct list_pane_funcs *funcs;
	void *data;  // for funcs
	char *title;
	unsigned int window_offset, selected;
	int active;
	unsigned int num_entries;
	WINDOW *subwin;
};

void UI_ListPaneInit(struct list_pane *p, WINDOW *w,
                     const struct list_pane_funcs *funcs, void *data);
void UI_ListPaneKeypress(void *p, int key);
void UI_ListPaneFree(struct list_pane *p);
int UI_ListPaneSelected(struct list_pane *p);
unsigned int UI_ListPaneLines(struct list_pane *lp);
void UI_ListPaneSetTitle(struct list_pane *lp, const char *title);
void UI_ListPaneSelect(struct list_pane *p, unsigned int idx);

#endif /* #ifndef INCLUDE_LIST_PANE_H */

