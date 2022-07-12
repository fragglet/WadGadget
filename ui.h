
#include "pane.h"
#include "text_input.h"

#define FILE_PANE_WIDTH  27
#define FILE_PANE_HEIGHT 24

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

void UI_InitHeaderPane(struct pane *pane, WINDOW *win);
void UI_InitInfoPane(struct pane *pane, WINDOW *win);
void UI_InitSearchPane(struct search_pane *pane, WINDOW *win);

