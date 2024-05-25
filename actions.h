//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "directory_pane.h"

struct action {
	int key;
	int ctrl_key;
	char *shortname;
	char *description;
	void (*callback)(struct directory_pane *active,
	                 struct directory_pane *other);
};

extern const struct action copy_action;
extern const struct action copy_noconv_action;
extern const struct action export_action;
extern const struct action export_noconv_action;
extern const struct action import_action;
extern const struct action import_noconv_action;
extern const struct action make_wad_action;
extern const struct action make_wad_noconv_action;
extern const struct action export_wad_action;
extern const struct action export_wad_action;

extern const struct action rename_action;
extern const struct action delete_action;
extern const struct action mark_pattern_action;
extern const struct action unmark_all_action;
extern const struct action mark_action;

extern const struct action quit_action;
extern const struct action redraw_screen_action;
extern const struct action reload_action;

extern const struct action view_action;
extern const struct action rearrange_action;
extern const struct action new_lump_action;
extern const struct action mkdir_action;
extern const struct action update_action;
