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

#include "pane.h"
#include "vfs.h"

struct action {
	char *key;
	char *description;
};

struct actions_pane {
	struct pane pane;
	int left_to_right;
	enum file_type active, other;
};

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void UI_ActionsPaneSet(struct actions_pane *pane, enum file_type active,
                       enum file_type other, int left_to_right);
