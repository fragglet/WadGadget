#ifndef INCLUDE_LIST_PANE_H
#define INCLUDE_LIST_PANE_H

#include "pane.h"
#include "blob_list.h"

enum list_pane_type {
	PANE_TYPE_NONE,
	PANE_TYPE_DIR,
	PANE_TYPE_WAD,
};

struct list_pane_action {
	char *key;
	char *description;
};

struct list_pane {
	struct pane pane;
	struct blob_list *blob_list;
	enum list_pane_type type;
	unsigned int window_offset, selected;
	const struct list_pane_action *(*get_actions)(struct list_pane *other);
};

struct actions_pane {
	struct pane pane;
	const struct list_pane_action *actions;
	int left_to_right;
};

void UI_ListPaneInit(struct list_pane *p, WINDOW *w);
const struct list_pane_action *UI_ListPaneActions(
	struct list_pane *p, struct list_pane *other);
enum blob_type UI_ListPaneEntryType(struct list_pane *p, unsigned int idx);
const char *UI_ListPaneEntryPath(struct list_pane *p, unsigned int idx);
void UI_ListPaneFree(struct list_pane *p);

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);

#endif /* #ifndef INCLUDE_LIST_PANE_H */

