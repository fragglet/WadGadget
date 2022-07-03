#include "wad_file.h"

struct wad_pane;

struct wad_pane *UI_NewWadPane(WINDOW *pane, struct wad_file *f);
void UI_DrawWadPane(struct wad_pane *p);
