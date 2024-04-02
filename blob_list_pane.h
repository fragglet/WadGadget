#ifndef INCLUDE_BLOB_LIST_PANE_H
#define INCLUDE_BLOB_LIST_PANE_H

#include "list_pane.h"
#include "blob_list.h"

enum blob_list_pane_type {
	PANE_TYPE_NONE,
	PANE_TYPE_DIR,
	PANE_TYPE_WAD,
};

struct blob_list_pane_action {
	char *key;
	char *description;
};

struct blob_list_pane {
	struct list_pane pane;
	struct blob_list *blob_list;
	enum blob_list_pane_type type;
	const struct blob_list_pane_action *(*get_actions)(
		struct blob_list_pane *other);
};

struct actions_pane {
	struct pane pane;
	const struct blob_list_pane_action *actions;
	int left_to_right;
};

void UI_BlobListPaneInit(struct blob_list_pane *p, WINDOW *w);
const struct blob_list_pane_action *UI_BlobListPaneActions(
	struct blob_list_pane *p, struct blob_list_pane *other);
enum blob_type UI_BlobListPaneEntryType(struct blob_list_pane *p, int idx);
const char *UI_BlobListPaneEntryPath(struct blob_list_pane *p, int idx);
void UI_BlobListPaneKeypress(void *p, int key);
void UI_BlobListPaneFree(struct blob_list_pane *p);
void UI_BlobListPaneSearch(void *p, char *needle);

int UI_BlobListPaneSelected(struct blob_list_pane *p);

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);

#endif /* #ifndef INCLUDE_LIST_PANE_H */

