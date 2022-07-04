
struct directory_pane;

struct directory_pane *UI_NewDirectoryPane(WINDOW *pane, const char *path);
void UI_DrawDirectoryPane(struct directory_pane *p);
