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

#include "common.h"
#include "fs/vfs.h"
#include "lump_info.h"
#include "sixel_display.h"
#include "stringlib.h"
#include "termfuncs.h"
#include "textures/textures.h"
#include "ui/actions_bar.h"
#include "ui/colors.h"
#include "ui/dialog.h"
#include "ui/stack.h"
#include "ui/title_bar.h"
#include "ui/ui.h"
#include "view.h"

#include "browser/actions_pane.h"
#include "browser/actions.h"
#include "browser/directory_pane.h"
#include "browser/browser.h"

#define INFO_PANE_WIDTH 30

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

static const struct action *actions[MAX_KEY_BINDINGS + 1];
static struct actions_pane actions_pane;
static struct pane info_pane;
static struct search_pane search_pane;
static WINDOW *pane_windows[2];
static bool cmdr_mode = false, use_function_keys = true;

struct directory_pane *browser_panes[2];
unsigned int current_pane = 0;

static void SetNwtWindowSizes(void)
{
	int left_width, right_width;
	int top_line, bottom_line, lines;

	UI_ActionsBarEnable(false);
	UI_GetDesktopLines(&top_line, &bottom_line);
	lines = bottom_line - top_line + 1;

	// Note minor adjustments here because the borders of the
	// panes overlap one another.
	left_width = (max(COLS, 80) - INFO_PANE_WIDTH + 1) / 2;
	right_width = max(COLS, 80) - left_width - INFO_PANE_WIDTH + 2;

	wresize(info_pane.window, 5, INFO_PANE_WIDTH);
	mvwin(info_pane.window, top_line, left_width - 1);
	wresize(actions_pane.pane.window, 18, INFO_PANE_WIDTH);
	mvwin(actions_pane.pane.window, top_line + 4, left_width - 1);
	wresize(search_pane.pane.window, 3, INFO_PANE_WIDTH);
	mvwin(search_pane.pane.window, bottom_line - 2,
	      left_width - 1);

	UI_PaneShow(&actions_pane);

	wresize(pane_windows[0], lines, left_width);
	mvwin(pane_windows[0], top_line, 0);
	wresize(pane_windows[1], lines, right_width);
	mvwin(pane_windows[1], top_line, COLS - right_width);
}

// Commander mode will eventually be an alternative look that emulates
// the look of Norton Commander & its clones. It's incomplete and for
// now just used for small window sizes.
static void SetCmdrWindowSizes(void)
{
	int left_width = COLS / 2;
	int right_width = COLS - left_width + 1;
	int top_line, bottom_line, lines;

	UI_ActionsBarEnable(true);
	UI_GetDesktopLines(&top_line, &bottom_line);
	lines = bottom_line - top_line + 1;

	wresize(info_pane.window, 5,
	        current_pane != 0 ? right_width : left_width);
	mvwin(info_pane.window, bottom_line - 5,
	      current_pane != 0 ? left_width - 1 : 0);

	UI_PaneHide(&actions_pane);

	// Search pane fits along bottom of screen.
	wresize(search_pane.pane.window, 1, COLS);
	mvwin(search_pane.pane.window, bottom_line, 0);

	wresize(pane_windows[0], lines - (current_pane ? 1 : 5),
	        left_width);
	mvwin(pane_windows[0], top_line, 0);
	wresize(pane_windows[1], lines - (current_pane ? 5 : 1),
	        right_width);
	mvwin(pane_windows[1], top_line, left_width - 1);

	// TODO: nc-style function keys row
}

static void SetWindowSizes(void)
{
	if (!cmdr_mode && COLS >= 80 && LINES >= 25) {
		SetNwtWindowSizes();
	} else {
		SetCmdrWindowSizes();
	}

	// Panes must be in order so that titles are shown.
	UI_RaisePaneToTop(&info_pane);
	UI_RaisePaneToTop(&actions_pane);
	// Search pane is always at the top to catch keypresses:
	UI_RaisePaneToTop(&search_pane);
}

static void PerformSwitchPane(void)
{
	B_SwitchToPane(other_pane);
}

static const struct action other_pane_action = {
	'\t', 0, "Other", "> Other pane",
	PerformSwitchPane,
};

static void ToggleCmdrMode(void)
{
	cmdr_mode = !cmdr_mode;
	if (!cmdr_mode) {
		use_function_keys = !use_function_keys;
	}
	B_ActionsPaneSet(&actions_pane, actions, current_pane == 0,
	                 use_function_keys);
	UI_ActionsBarSetFunctionKeys(use_function_keys);
	UI_ActionsBarSetActions(actions);
	UI_ActionsBarEnable(true);
	SetWindowSizes();
}

static const struct action cmdr_mode_action = {
	0, 'J', "CmdrMode", "| CmdrMode",
	ToggleCmdrMode,
};

static void SearchAgain(void)
{
	int old_selected = active_pane->pane.selected;
	bool found;

	found = B_DirectoryPaneSearchAgain(active_pane,
	                                   search_pane.input.input);

	if (active_pane->pane.selected < old_selected) {
		UI_ShowNotice("Searched to the end; returning to the start.");
	} else if (active_pane->pane.selected > old_selected) {
		// We found another match. Great.
	} else if (found) {
		UI_ShowNotice("No other matches found.");
	} else {
		UI_ShowNotice("No matches found.");
	}
}

static const struct action search_again_action = {
	0, 'N', "Next", "Search again",
	SearchAgain,
};

static void SwapPanes(void)
{
	struct directory_pane *tmp = browser_panes[0];
	WINDOW *wintmp = pane_windows[0];

	browser_panes[0] = browser_panes[1];
	browser_panes[1] = tmp;
	pane_windows[0] = pane_windows[1];
	pane_windows[1] = wintmp;

	B_SwitchToPane(active_pane);
}

static const struct action swap_panes_action = {
	0, '_', "Swap", "Swap panes",
	SwapPanes,
};

static const struct action *wad_actions[] = {
	&rearrange_action,
	&new_lump_action,
	&edit_action,
	&undo_action,
	&redo_action,
	&sort_entries_action,
	&hexdump_action,
	&open_palettes_action,
	NULL,
};

static const struct action *dir_actions[] = {
	&compact_action,
	&open_shell_action,
	&make_wad_action,
	&make_wad_noconv_action,
	&mkdir_action,
	&hexdump_action,
	&open_palettes_action,
	NULL,
};

static const struct action *txt_actions[] = {
	&edit_textures_action,
	&rearrange_action,
	&sort_entries_action,
	&new_texture_action,
	&dup_texture_action,
	&undo_action,
	&redo_action,
	NULL,
};

static const struct action *pnm_actions[] = {
	&edit_pnames_action,
	&rearrange_action,
	&sort_entries_action,
	&new_pname_action,
	&undo_action,
	&redo_action,
	NULL,
};

static const struct action *pal_actions[] = {
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

static const struct action *wad_to_pnm[] = {
	&copy_pnames_action,
	NULL,
};

static const struct action *wad_to_pal[] = {
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

static const struct action *dir_to_pnm[] = {
	&import_texture_config,
	NULL,
};

static const struct action *dir_to_pal[] = {
	NULL,
};

static const struct action *dir_to_txt[] = {
	&import_texture_config,
	NULL,
};

static const struct action *txt_to_dir[] = {
	&export_texture_config,
	NULL,
};

static const struct action *txt_to_txt[] = {
	&copy_textures_action,
	NULL,
};

static const struct action *pnm_to_pnm[] = {
	&copy_pnames_action,
	NULL,
};

static const struct action *pnm_to_dir[] = {
	&export_texture_config,
	NULL,
};

static const struct action *pal_to_dir[] = {
	NULL,
};

static const struct action *pal_to_wad[] = {
	NULL,
};

static const struct action *no_actions[] = {NULL};

static const struct action *common_actions[] = {
	&rename_action,
	&delete_action,
	&mark_pattern_action,
	&unmark_all_action,
	&cmdr_mode_action,
	&swap_panes_action,
	&search_again_action,
	&reload_action,
	&mark_action,
	&delete_no_confirm_action,
	&view_action,
	&other_pane_action,
	&help_action,
	&quit_action,
	NULL,
};

static const struct action **type_actions[NUM_DIR_FILE_TYPES] = {
	dir_actions, wad_actions, txt_actions, pnm_actions, pal_actions,
};

static const struct action
    **action_lists[NUM_DIR_FILE_TYPES][NUM_DIR_FILE_TYPES] = {
	{dir_to_dir, dir_to_wad, dir_to_txt, dir_to_pnm, dir_to_pal},
	{wad_to_dir, wad_to_wad, no_actions, wad_to_pnm, wad_to_pal},
	{txt_to_dir, no_actions, txt_to_txt, no_actions, no_actions},
	{pnm_to_dir, no_actions, no_actions, pnm_to_pnm, no_actions},
	{pal_to_dir, pal_to_wad, no_actions, no_actions, no_actions},
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
	int active = active_pane->dir->type;
	int other = other_pane->dir->type;
	int idx = 0;

	assert(active < NUM_DIR_FILE_TYPES);
	assert(other < NUM_DIR_FILE_TYPES);

	memset(actions, 0, sizeof(struct action *) * MAX_KEY_BINDINGS);

	AddActionList(type_actions[active], &idx);
	AddActionList(action_lists[active][other], &idx);
	AddActionList(common_actions, &idx);
	actions[idx] = NULL;
}

static int PaneNum(struct directory_pane *p)
{
	unsigned int result = p == browser_panes[0] ? 0 : 1;
	assert(browser_panes[result] == p);
	return result;
}

void B_SwitchToPane(struct directory_pane *pane)
{
	unsigned int pane_num = PaneNum(pane);

	active_pane->pane.active = 0;
	current_pane = pane_num;
	UI_RaisePaneToTop(pane);
	pane->pane.active = 1;

	BuildActionsList();
	B_ActionsPaneSet(&actions_pane, actions, current_pane == 0,
	                 use_function_keys);
	UI_ActionsBarSetActions(actions);
	SetWindowSizes();
}

void B_ReplacePane(struct directory_pane *old_pane,
                   struct directory_pane *new_pane)
{
	int pane_num = PaneNum(old_pane);

	// New pane gets old pane's window.
	new_pane->pane.pane.window = old_pane->pane.pane.window;

	VFS_CloseDir(old_pane->dir);
	UI_PaneHide(old_pane);
	// TODO B_DirectoryPaneFree(old_pane);

	browser_panes[pane_num] = new_pane;
	UI_PaneShow(new_pane);

	if (pane_num == current_pane) {
		B_SwitchToPane(new_pane);
	}

	// TODO: Does this belong here?
	UI_TextInputClear(&search_pane.input);
}

// Drag and drop paste: if the user types a fully-qualified path into the
// search box and then types a space, we navigate to that directory.
bool B_CheckPathPaste(void)
{
	const char *input = search_pane.input.input;
	struct directory *dir;
	struct directory_pane *new_pane;

	if (strlen(input) == 0 || input[0] != '/') {
		return false;
	}

	dir = VFS_OpenDir(input);
	if (dir == NULL) {
		return false;
	}

	new_pane = UI_NewDirectoryPane(NULL, dir);
	B_ReplacePane(active_pane, new_pane);

	UI_TextInputClear(&search_pane.input);

	return true;
}

static void HandleKeypress(void *pane, int key)
{
	switch (key) {
	case KEY_LEFT:
		B_SwitchToPane(browser_panes[0]);
		break;
	case KEY_RIGHT:
		B_SwitchToPane(browser_panes[1]);
		break;
	case KEY_RESIZE:
		SetWindowSizes();
		break;
	default:
		UI_PaneKeypress(active_pane, key);
		break;
	}
}

static bool DrawInfoPane(void *p)
{
	struct directory *dir;
	struct directory_entry *ent;
	struct textures *txs;
	struct texture *t;
	struct pane *pane = p;
	int idx = B_DirectoryPaneSelected(active_pane);
	const struct lump_type *lt;
	struct wad_file *wf;
	char buf[10], buf2[64];

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	UI_DrawWindowBox(pane->window);
	mvwaddstr(pane->window, 0, 2, " Info ");

	if (idx < 0) {
		return true;
	}
	dir = active_pane->dir;
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
		if (strlen(buf) > 0) {
			snprintf(buf2, sizeof(buf2), "Size: %sB", buf);
			UI_PrintMultilineString(pane->window, 2, 2, buf2);
		}
		break;

	case FILE_TYPE_DIR:
		UI_PrintMultilineString(pane->window, 1, 2,
		    "Directory");
		break;

	case FILE_TYPE_TEXTURE_LIST:
	case FILE_TYPE_PNAMES_LIST:
	case FILE_TYPE_PALETTES:
		UI_PrintMultilineString(pane->window, 1, 2, "List");
		break;

	case FILE_TYPE_TEXTURE:
		txs = TX_TextureList(dir);
		t = txs->textures[idx];
		snprintf(buf2, sizeof(buf2),
		         "Texture\nDimensions: %dx%d\nPatches: %d",
		         t->width, t->height, t->patchcount);
		UI_PrintMultilineString(pane->window, 1, 2, buf2);
		break;

	case FILE_TYPE_PNAME:
		UI_PrintMultilineString(pane->window, 1, 2, "Patch name");
		break;

	case NUM_DIR_FILE_TYPES:
		assert(0);
	}

	return true;
}

static bool DrawSearchPane(void *pane)
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

	return true;
}

static void SearchPaneKeypress(void *pane, int key)
{
	struct search_pane *p = pane;

	// Space key triggers mark, does not go to search input.
	if (key != ' ' && UI_TextInputKeypress(&p->input, key)) {
		if (key != KEY_BACKSPACE) {
			B_DirectoryPaneSearch(active_pane, p->input.input);
		}
	} else {
		HandleKeypress(NULL, key);
	}
}

static void InitInfoPane(WINDOW *win)
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
	UI_TextInputInit(&search_pane.input, win, 256);
}

void B_Shutdown(void)
{
	TF_RestoreOldPalette();
	if (browser_panes[0] != NULL) {
		VFS_CloseDir(browser_panes[0]->dir);
	}
	if (browser_panes[1] != NULL) {
		VFS_CloseDir(browser_panes[1]->dir);
	}
	clear();
	refresh();
	endwin();
}

void B_Init(const char *path1, const char *path2)
{
	struct directory *dir;

	InitInfoPane(newwin(5, 26, 1, 27));
	UI_PaneShow(&info_pane);

	InitSearchPane(newwin(4, 26, 20, 27));
	UI_PaneShow(&search_pane);

	B_ActionsPaneInit(&actions_pane, newwin(15, 26, 6, 27));
	UI_PaneShow(&actions_pane);

	pane_windows[0] = newwin(24, 27, 1, 0);
	dir = VFS_OpenDir(path1);
	if (dir == NULL) {
		B_Shutdown();
		fprintf(stderr, "Failed to open '%s'.\n", path1);
		exit(-1);
	}
	browser_panes[0] = UI_NewDirectoryPane(pane_windows[0], dir);
	UI_PaneShow(browser_panes[0]);

	pane_windows[1] = newwin(24, 27, 1, 53);
	dir = VFS_OpenDir(path2);
	if (dir == NULL) {
		B_Shutdown();
		fprintf(stderr, "Failed to open '%s'.\n", path2);
		exit(-1);
	}
	browser_panes[1] = UI_NewDirectoryPane(pane_windows[1], dir);
	UI_PaneShow(browser_panes[1]);

	B_SwitchToPane(browser_panes[0]);

	SetWindowSizes();
}
