//
// Copyright(C) 2022-2024 Simon Howard
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef INCLUDE_DIRECTORY_PANE_H
#define INCLUDE_DIRECTORY_PANE_H

#include "list_pane.h"
#include "vfs.h"

struct directory_pane {
	struct list_pane pane;
	int left_to_right;
	struct directory *dir;
	struct file_set tagged;
};

enum file_type UI_DirectoryPaneEntryType(struct directory_pane *p);
char *UI_DirectoryPaneEntryPath(struct directory_pane *p);
void UI_DirectoryPaneKeypress(void *p, int key);
void UI_DirectoryPaneFree(struct directory_pane *p);
void UI_DirectoryPaneSearch(void *p, const char *needle);
int UI_DirectoryPaneSelected(struct directory_pane *p);
struct file_set *UI_DirectoryPaneTagged(struct directory_pane *p);
void UI_DirectoryPaneSetTagged(struct directory_pane *p, struct file_set *set);

struct directory_pane *UI_NewDirectoryPane(
	WINDOW *pane, struct directory *dir);

#endif /* #ifndef INCLUDE_DIRECTORY_PANE_H */
