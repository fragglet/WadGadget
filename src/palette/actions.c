//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ui/actions_bar.h"
#include "browser/actions.h"
#include "browser/browser.h"
#include "browser/directory_pane.h"
#include "view.h"

#include "palette/palette.h"
#include "palette/palfs.h"

static void PerformViewPalette(void)
{
	struct directory_entry *ent = B_DirectoryPaneEntry(active_pane);

	if (ent->type == FILE_TYPE_PALETTE) {
		OpenDirent(PAL_InnerDir(active_pane->dir),
		           PAL_InnerEntry(active_pane->dir, ent), false);
		return;
	}

	// Otherwise, do the normal "View" action thing:
	view_action.callback();
}

const struct action view_palette_action = {
	'\r', 0,  "View", "View",
	PerformViewPalette,
};

static void PerformSetDefault(void)
{
	struct directory_entry *ent = B_DirectoryPaneEntry(active_pane);

	if (ent->type != FILE_TYPE_PALETTE) {
		return;
	}

	ent = PAL_InnerEntry(active_pane->dir, ent);

	PAL_SetDefaultPointer(ent->name);
	VFS_Refresh(active_pane->dir);
}

const struct action set_default_palette_action = {
	KEY_F(4), 'D',  "SetDef", "Set default",
	PerformSetDefault,
};
