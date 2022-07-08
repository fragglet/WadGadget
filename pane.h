#ifndef PANE_H_INCLUDED
#define PANE_H_INCLUDED

#include <curses.h>

struct pane {
	WINDOW *window;
	void (*draw)(void *pane);
	void (*keypress)(void *pane, int key);
	unsigned int active;
};

void UI_PaneActive(void *pane, int active);
void UI_PaneDraw(void *pane);
void UI_PaneKeypress(void *pane, int key);
void UI_PaneShow(void *pane);
void UI_PaneHide(void *pane);
void UI_DrawAllPanes(void);

#endif /* #ifndef PANE_H_INCLUDED */

