//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "actions_pane.h"
#include "colors.h"
#include "common.h"
#include "dialog.h"
#include "directory_pane.h"
#include "export.h"
#include "import.h"
#include "lump_info.h"
#include "sixel_display.h"
#include "strings.h"
#include "termfuncs.h"
#include "ui.h"
#include "view.h"

#define VERSION_OUTPUT \
"WadGadget version ?\n" \
"Copyright (C) 2022-2024 Simon Howard\n" \
"License GPLv2+: GNU GPL version 2 or later:\n" \
"<https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n" \
"\n" \
"This is free software; see COPYING.md for copying conditions. There is NO\n" \
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

#define INFO_PANE_WIDTH 29

void SwitchToPane(struct directory_pane *pane);

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

static const struct action *actions[MAX_KEY_BINDINGS + 1];
static struct actions_pane actions_pane;
static struct actions_bar actions_bar;
static struct pane header_pane, info_pane;
static struct search_pane search_pane;
static WINDOW *pane_windows[2];
static struct directory_pane *panes[2];
static bool cmdr_mode = false, use_function_keys = true;
static unsigned int active_pane = 0;

static void SetNwtWindowSizes(int columns, int lines)
{
	int left_width, right_width;

	// Note minor adjustments here because the borders of the
	// panes overlap one another.
	left_width = (max(columns, 80) - INFO_PANE_WIDTH + 1) / 2;
	right_width = max(columns, 80) - left_width - INFO_PANE_WIDTH + 2;

	wresize(info_pane.window, 5, INFO_PANE_WIDTH);
	mvwin(info_pane.window, 1, left_width - 1);
	wresize(actions_pane.pane.window, 18, INFO_PANE_WIDTH);
	mvwin(actions_pane.pane.window, 5, left_width - 1);
	wresize(search_pane.pane.window, 3, INFO_PANE_WIDTH);
	mvwin(search_pane.pane.window, lines - 3,
	      left_width - 1);

	UI_PaneHide(&actions_bar);
	UI_PaneShow(&actions_pane);

	wresize(pane_windows[0], lines - 1, left_width);
	mvwin(pane_windows[0], 1, 0);
	wresize(pane_windows[1], lines - 1, right_width);
	mvwin(pane_windows[1], 1, columns - right_width);
}

// Commander mode will eventually be an alternative look that emulates
// the look of Norton Commander & its clones. It's incomplete and for
// now just used for small window sizes.
static void SetCmdrWindowSizes(int columns, int lines)
{
	int left_width = columns / 2;
	int right_width = columns - left_width + 1;

	wresize(info_pane.window, 5,
	        active_pane != 0 ? right_width : left_width);
	mvwin(info_pane.window, lines - 7,
	      active_pane != 0 ? left_width - 1 : 0);

	UI_PaneHide(&actions_pane);
	UI_PaneShow(&actions_bar);

	// Search pane fits along bottom of screen.
	wresize(search_pane.pane.window, 1, columns);
	mvwin(search_pane.pane.window, lines - 2, 0);

	wresize(actions_bar.pane.window, 1, columns);
	mvwin(actions_bar.pane.window, lines - 1, 0);

	wresize(pane_windows[0], lines - (active_pane ? 3 : 7),
	        left_width);
	mvwin(pane_windows[0], 1, 0);
	wresize(pane_windows[1], lines - (active_pane ? 7 : 3),
	        right_width);
	mvwin(pane_windows[1], 1, left_width - 1);

	// TODO: nc-style function keys row
}

static void SetWindowSizes(void)
{
	int lines, columns;
	getmaxyx(stdscr, lines, columns);

	wresize(header_pane.window, 1, columns);

	if (!cmdr_mode && columns >= 80) {
		SetNwtWindowSizes(columns, lines);
	} else {
		SetCmdrWindowSizes(columns, lines);
	}

	// Panes must be in order so that titles are shown.
	UI_RaisePaneToTop(&info_pane);
	UI_RaisePaneToTop(&actions_pane);
	// Search pane is always at the top to catch keypresses:
	UI_RaisePaneToTop(&search_pane);
}

static void PerformSwitchPane(struct directory_pane *a,
                              struct directory_pane *other_pane)
{
	SwitchToPane(other_pane);
}

static const struct action other_pane_action = {
	'\t', 0, "Other", "> Other pane",
	PerformSwitchPane,
};

static void ToggleCmdrMode(struct directory_pane *a,
                           struct directory_pane *b)
{
	cmdr_mode = !cmdr_mode;
	if (!cmdr_mode) {
		use_function_keys = !use_function_keys;
	}
	UI_ActionsPaneSet(&actions_pane, actions, active_pane == 0,
	                  use_function_keys);
	UI_ActionsBarSet(&actions_bar, actions, use_function_keys);
	SetWindowSizes();
}

static const struct action cmdr_mode_action = {
	0, 'J', "CmdrMode", "| CmdrMode",
	ToggleCmdrMode,
};

static void SearchAgain(struct directory_pane *active_pane,
                        struct directory_pane *b)
{
	UI_DirectoryPaneSearchAgain(active_pane, search_pane.input.input);
}

static const struct action search_again_action = {
	0, 'N', "Next", "Search again",
	SearchAgain,
};

static const struct action *wad_actions[] = {
	&rearrange_action,
	&new_lump_action,
	&edit_action,
	NULL,
};

static const struct action *dir_actions[] = {
	&compact_action,
	&make_wad_action,
	&make_wad_noconv_action,
	&mkdir_action,
	NULL,
};

static const struct action *wad_to_wad[] = {
	&update_action,
	&copy_action,
	NULL,
};

static const struct action *wad_to_dir[] = {
	&export_wad_action,
	&export_action,
	&export_noconv_action,
	NULL,
};

static const struct action *dir_to_wad[] = {
	&update_action,
	&import_action,
	&import_noconv_action,
	NULL,
};

static const struct action *dir_to_dir[] = {
	&copy_action,
	NULL,
};

static const struct action *common_actions[] = {
	&rename_action,
	&delete_action,
	&mark_action,
	&mark_pattern_action,
	&unmark_all_action,
	&cmdr_mode_action,
	&search_again_action,
	&hexdump_action,
	&redraw_screen_action,
	&reload_action,
	&view_action,
	&other_pane_action,
	&quit_action,
	NULL,
};

static const struct action **type_actions[2] = {
	dir_actions, wad_actions,
};

static const struct action **action_lists[2][2] = {
	{dir_to_dir, dir_to_wad},
	{wad_to_dir, wad_to_wad},
};

static void AddActionList(const struct action **list, int *idx)
{
	int i = 0;

	for (i = 0; list[i] != NULL && *idx < MAX_KEY_BINDINGS; i++) {
		actions[*idx] = list[i];
		++*idx;
	}
}

static void BuildActionsList(void)
{
	int active = panes[active_pane]->dir->type;
	int other = panes[!active_pane]->dir->type;
	int idx = 0;

	memset(actions, 0, sizeof(struct action *) * MAX_KEY_BINDINGS);

	AddActionList(common_actions, &idx);
	AddActionList(type_actions[active == FILE_TYPE_WAD], &idx);
	AddActionList(
		action_lists[active == FILE_TYPE_WAD][other == FILE_TYPE_WAD],
		&idx);
	actions[idx] = NULL;
}

static int PaneNum(struct directory_pane *p)
{
	unsigned int result = p == panes[0] ? 0 : 1;
	assert(panes[result] == p);
	return result;
}

void SwitchToPane(struct directory_pane *pane)
{
	unsigned int pane_num = PaneNum(pane);

	panes[active_pane]->pane.active = 0;
	active_pane = pane_num;
	UI_RaisePaneToTop(pane);
	pane->pane.active = 1;

	BuildActionsList();
	UI_ActionsPaneSet(&actions_pane, actions, active_pane == 0,
	                  use_function_keys);
	UI_ActionsBarSet(&actions_bar, actions, use_function_keys);
	SetWindowSizes();
}

void ReplacePane(struct directory_pane *old_pane,
                 struct directory_pane *new_pane)
{
	int pane_num = PaneNum(old_pane);

	// New pane gets old pane's window.
	new_pane->pane.pane.window = old_pane->pane.pane.window;

	VFS_CloseDir(old_pane->dir);
	UI_PaneHide(old_pane);
	// TODO UI_DirectoryPaneFree(old_pane);

	panes[pane_num] = new_pane;
	UI_PaneShow(new_pane);

	// TODO: Does this belong here?
	UI_TextInputClear(&search_pane.input);
}

static void HandleKeypress(void *pane, int key)
{
	int i;

	// A couple of hacks because actions only support two bindings.
	if (key == KEY_IC) { // ins
		key = KEY_F(7);
	} else if (key == KEY_DC) { // del
		key = KEY_F(8);
	}

	for (i = 0; actions[i] != NULL; i++) {
		if (actions[i]->callback != NULL
		 && (key == actions[i]->key
		  || key == CTRL_(actions[i]->ctrl_key))) {
			actions[i]->callback(panes[active_pane],
			                     panes[!active_pane]);
			return;
		}
	}

	switch (key) {
	case KEY_LEFT:
		SwitchToPane(panes[0]);
		break;
	case KEY_RIGHT:
		SwitchToPane(panes[1]);
		break;
	case KEY_RESIZE:
		SetWindowSizes();
		break;
	default:
		UI_PaneKeypress(panes[active_pane], key);
		break;
	}
}

static void DrawInfoPane(void *p)
{
	struct directory *dir;
	struct directory_entry *ent;
	struct pane *pane = p;
	int idx = UI_DirectoryPaneSelected(panes[active_pane]);
	const struct lump_type *lt;
	struct wad_file *wf;
	char buf[10], buf2[30];

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	UI_DrawWindowBox(pane->window);
	mvwaddstr(pane->window, 0, 2, " Info ");

	if (idx < 0) {
		return;
	}
	dir = panes[active_pane]->dir;
	ent = &dir->entries[idx];
	switch (ent->type) {
	case FILE_TYPE_LUMP:
		wf = VFS_WadFile(dir);
		lt = LI_IdentifyLump(wf, idx);
		UI_PrintMultilineString(pane->window, 1, 2,
		    LI_DescribeLump(lt, wf, idx));
		break;

	case FILE_TYPE_FILE:
	case FILE_TYPE_WAD:
		UI_PrintMultilineString(pane->window, 1, 2, "File\n");
		VFS_DescribeSize(ent, buf, false);
		snprintf(buf2, sizeof(buf2), "Size: %sB", buf);
		UI_PrintMultilineString(pane->window, 2, 2, buf2);
		break;

	case FILE_TYPE_DIR:
		UI_PrintMultilineString(pane->window, 1, 2,
		    "Directory");
		break;
	}
}

static void DrawSearchPane(void *pane)
{
	struct search_pane *p = pane;
	WINDOW *win = p->pane.window;
	int w = getmaxx(win);

	if (getmaxy(win) > 1) {
		wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
		werase(win);
		UI_DrawWindowBox(win);
		mvwaddstr(win, 0, 2, " Search ");
		mvderwin(p->input.win, 1, 2);
		wresize(p->input.win, 1, w - 4);
		if (strlen(p->input.input) > 0) {
			mvwaddstr(win, 0, w - 13, "[   - Next]");
			wattron(win, A_BOLD);
			mvwaddstr(win, 0, w - 12, "^N");
			wattroff(win, A_BOLD);
		}
	} else {
		wbkgdset(win, COLOR_PAIR(PAIR_WHITE_BLACK));
		werase(win);
		mvwaddstr(win, 0, 0, " Search: ");
		mvderwin(p->input.win, 0, 9);
		wresize(p->input.win, 1, w - 9);
	}
	UI_TextInputDraw(&p->input);
}

static void SearchPaneKeypress(void *pane, int key)
{
	struct search_pane *p = pane;

	// Space key triggers mark, does not go to search input.
	if (key != ' ' && UI_TextInputKeypress(&p->input, key)) {
		if (key != KEY_BACKSPACE) {
			UI_DirectoryPaneSearch(panes[active_pane],
			                       p->input.input);
		}
	} else {
		HandleKeypress(NULL, key);
	}
}

void InitInfoPane(WINDOW *win)
{
	info_pane.window = win;
	info_pane.draw = DrawInfoPane;
	info_pane.keypress = NULL;
}

static void InitSearchPane(WINDOW *win)
{
	search_pane.pane.window = win;
	search_pane.pane.draw = DrawSearchPane;
	search_pane.pane.keypress = SearchPaneKeypress;
	UI_TextInputInit(&search_pane.input, win, 20);
}

static void Shutdown(void)
{
	TF_RestoreOldPalette();
	clear();
	refresh();
	endwin();
}

int main(int argc, char *argv[])
{
	struct directory *dir;
	const char *start_path1 = ".", *start_path2 = ".";

#ifdef SIGIO
	signal(SIGIO, SIG_IGN);
#endif
#ifdef SIGPOLL
	signal(SIGPOLL, SIG_IGN);
#endif

	SIXEL_CheckSupported();

	if (argc == 2 && !strcmp(argv[1], "--version")) {
		printf(VERSION_OUTPUT);
		exit(0);
	}
	if (argc >= 2) {
		start_path1 = argv[1];
	}
	if (argc >= 3) {
		start_path2 = argv[2];
	}

	initscr();
	start_color();
	TF_SetNewPalette();
	TF_SetCursesModes();
	TF_SetColorPairs();

	refresh();

	// The hard-coded window sizes and positions here get reset
	// when SetWindowSizes() is called below.
	UI_InitHeaderPane(&header_pane, newwin(1, 80, 0, 0));
	UI_PaneShow(&header_pane);

	InitInfoPane(newwin(5, 26, 1, 27));
	UI_PaneShow(&info_pane);

	InitSearchPane(newwin(4, 26, 20, 27));
	UI_PaneShow(&search_pane);

	UI_ActionsPaneInit(&actions_pane, newwin(15, 26, 6, 27));
	UI_PaneShow(&actions_pane);

	UI_ActionsBarInit(&actions_bar, newwin(1, 1, 1, 1));
	UI_PaneShow(&actions_bar);

	pane_windows[0] = newwin(24, 27, 1, 0);
	dir = VFS_OpenDir(start_path1);
	if (dir == NULL) {
		Shutdown();
		fprintf(stderr, "Failed to open '%s'.\n", start_path1);
		exit(-1);
	}
	panes[0] = UI_NewDirectoryPane(pane_windows[0], dir);
	UI_PaneShow(panes[0]);

	if (dir->type == FILE_TYPE_WAD
	 && !strcmp(start_path1, start_path2)) {
		Shutdown();
		fprintf(stderr, "Can't open the same WAD in both panes.\n");
		exit(-1);
	}
	pane_windows[1] = newwin(24, 27, 1, 53);
	dir = VFS_OpenDir(start_path2);
	if (dir == NULL) {
		Shutdown();
		fprintf(stderr, "Failed to open '%s'.\n", start_path2);
		exit(-1);
	}
	panes[1] = UI_NewDirectoryPane(pane_windows[1], dir);
	UI_PaneShow(panes[1]);

	SwitchToPane(panes[0]);

	SetWindowSizes();
	UI_RunMainLoop();

	Shutdown();
}
