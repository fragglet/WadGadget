//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef BROWSER__DIRECTORY_PANE_H_INCLUDED
#define BROWSER__DIRECTORY_PANE_H_INCLUDED

#include "ui/list_pane.h"
#include "fs/vfs.h"

struct directory_pane {
	struct list_pane pane;
	int left_to_right;
	struct directory *dir;
	struct file_set tagged;
};

struct directory_entry *B_DirectoryPaneEntry(struct directory_pane *p);
void B_DirectoryPaneKeypress(void *p, int key);
void B_DirectoryPaneFree(struct directory_pane *p);
void B_DirectoryPaneSearch(void *p, const char *needle);
bool B_DirectoryPaneSearchAgain(void *p, const char *needle);
void B_DirectoryPaneReselect(struct directory_pane *p);
void B_DirectoryPaneSelectEntry(struct directory_pane *p,
                                struct directory_entry *ent);
void B_DirectoryPaneSelectBySerial(struct directory_pane *p,
                                   uint64_t serial_no);
void B_DirectoryPaneSelectByName(struct directory_pane *p, const char *name);
int B_DirectoryPaneSelected(struct directory_pane *p);
struct file_set *B_DirectoryPaneTagged(struct directory_pane *p);
void B_DirectoryPaneSetTagged(struct directory_pane *p, struct file_set *set);

struct directory_pane *UI_NewDirectoryPane(
	WINDOW *pane, struct directory *dir);

#endif /* #ifndef BROWSER__DIRECTORY_PANE_H_INCLUDED */
