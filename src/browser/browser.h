//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

void B_SwitchToPane(struct directory_pane *pane);
void B_ReplacePane(struct directory_pane *old_pane,
                   struct directory_pane *new_pane);
void B_Shutdown(void);
void B_Init(const char *path1, const char *path2);

extern struct directory_pane *browser_panes[2];
extern unsigned int current_pane;

#define active_pane  (browser_panes[current_pane])
#define other_pane   (browser_panes[!current_pane])

