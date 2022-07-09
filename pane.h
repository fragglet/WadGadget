#ifndef PANE_H_INCLUDED
#define PANE_H_INCLUDED

#include <curses.h>

struct pane {
	WINDOW *window;
	void (*draw)(void *pane, int active);
	void (*keypress)(void *pane, int key);
};

void UI_PaneKeypress(void *pane, int key);
void UI_PaneShow(void *pane);
int UI_PaneHide(void *pane);
void UI_DrawAllPanes(void);
void UI_RaisePaneToTop(void *pane);
void UI_RunMainLoop(void);
void UI_ExitMainLoop(void);

#endif /* #ifndef PANE_H_INCLUDED */

