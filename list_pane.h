#ifndef INCLUDE_LIST_PANE_H
#define INCLUDE_LIST_PANE_H

#include <curses.h>

#include "blob_list.h"

enum list_pane_type {
	PANE_TYPE_NONE,
	PANE_TYPE_DIR,
	PANE_TYPE_WAD,
};

enum list_pane_entry_type {
	PANE_ENTRY_FILE,
	PANE_ENTRY_DIR,
	PANE_ENTRY_LUMP,
};

struct list_pane_action {
	char *key;
	char *description;
};

struct list_pane {
	WINDOW *pane;
	struct blob_list *blob_list;
	enum list_pane_type type;
	unsigned int window_offset, selected;
	unsigned int active;
	const char *(*get_entry_str)(struct list_pane *p, unsigned int i);
	const struct list_pane_action *(*get_actions)(struct list_pane *other);
	enum list_pane_entry_type (*get_entry_type)(
		struct list_pane *p, unsigned int idx);
};

void UI_DrawListPane(struct list_pane *pane);
void UI_ListPaneInput(struct list_pane *p, int key);
void UI_ListPaneActive(struct list_pane *p, int active);
const struct list_pane_action *UI_ListPaneActions(
	struct list_pane *p, struct list_pane *other);
enum list_pane_entry_type UI_ListPaneEntryType(
	struct list_pane *p, unsigned int idx);

#endif /* #ifndef INCLUDE_LIST_PANE_H */

