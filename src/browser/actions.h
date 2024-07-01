//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef BROWSER_ACTIONS_H
#define BROWSER_ACTIONS_H

#include "browser/directory_pane.h"
#include "ui/actions_bar.h"

extern const struct action copy_action;
extern const struct action copy_noconv_action;
extern const struct action export_action;
extern const struct action export_noconv_action;
extern const struct action import_action;
extern const struct action import_noconv_action;
extern const struct action make_wad_action;
extern const struct action make_wad_noconv_action;
extern const struct action export_wad_action;

extern const struct action rename_action;
extern const struct action delete_action;
extern const struct action delete_no_confirm_action;
extern const struct action mark_pattern_action;
extern const struct action unmark_all_action;
extern const struct action mark_action;

extern const struct action quit_action;
extern const struct action redraw_screen_action;
extern const struct action reload_action;

extern const struct action rearrange_action;
extern const struct action sort_entries_action;
extern const struct action new_lump_action;
extern const struct action mkdir_action;
extern const struct action update_action;

extern const struct action help_action;
extern const struct action view_action;
extern const struct action hexdump_action;
extern const struct action edit_action;
extern const struct action compact_action;

extern const struct action undo_action;
extern const struct action redo_action;

bool B_CheckReadOnly(struct directory *dir);

#endif  /* #ifdef BROWSER_ACTIONS_H */
