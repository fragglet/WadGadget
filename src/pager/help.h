//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef PAGER_HELP_H
#define PAGER_HELP_H

struct help_pager_history {
	char *filename;
	int window_offset, current_link, current_column;
	struct help_pager_history *next;
};

struct help_pager_config {
	struct pager_config pc;
	char *filename;
	char **lines;
	struct pager_link *links;
	struct help_pager_history *history;
};

void P_FreeHelpConfig(struct help_pager_config *cfg);
bool P_InitHelpConfig(struct help_pager_config *cfg, const char *filename);
bool P_RunHelpPager(const char *filename);

#endif  /* #ifndef PAGER_HELP_H */
