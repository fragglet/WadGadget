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

#define INFO_PANE_WIDTH 28

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

static struct actions_pane actions_pane;
static struct actions_bar actions_bar;
static struct pane header_pane, info_pane;
static struct search_pane search_pane;
static WINDOW *pane_windows[2];
static struct directory_pane *panes[2];
static struct directory *dirs[2];
static bool cmdr_mode = false;
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

	// Disabled.
	wresize(actions_bar.pane.window, 1, 1);
	mvwin(actions_bar.pane.window, 1, 1);

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

	// This is a hack: shrink these to effectively "hide" them.
	wresize(actions_pane.pane.window, 1, 1);
	mvwin(actions_pane.pane.window, 1, 1);

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
}

static void SwitchToPane(unsigned int pane)
{
	panes[active_pane]->pane.active = 0;
	active_pane = pane;
	UI_RaisePaneToTop(panes[pane]);
	panes[active_pane]->pane.active = 1;
	// Info pane always above dir panes to preserve its title:
	UI_RaisePaneToTop(&info_pane);
	// Search pane is always at the top to catch keypresses:
	UI_RaisePaneToTop(&search_pane);

	UI_ActionsBarSet(&actions_bar, dirs[active_pane]->type,
	                 dirs[!active_pane]->type);
	UI_ActionsPaneSet(&actions_pane, dirs[active_pane]->type,
	                  dirs[!active_pane]->type, active_pane == 0);
	SetWindowSizes();
}

static void NavigateNew(void)
{
	struct directory_pane *pane = panes[active_pane];
	struct directory_pane *new_pane = NULL;
	struct directory *new_dir;
	const char *old_path;
	char *path;

	path = UI_DirectoryPaneEntryPath(pane);

	// Don't allow the same WAD to be opened twice.
	if (!strcmp(path, dirs[!active_pane]->path)
	 && dirs[!active_pane]->type == FILE_TYPE_WAD) {
		free(path);
		SwitchToPane(!active_pane);
		return;
	}

	new_dir = VFS_OpenDir(path);
	if (new_dir == NULL) {
		int idx = UI_DirectoryPaneSelected(pane);
		free(path);
		UI_MessageBox("Error when opening '%s'.",
		              dirs[active_pane]->entries[idx].name);
		return;
	}

	new_pane = UI_NewDirectoryPane(pane_windows[active_pane], new_dir);

	// Select subfolder we just navigated out of?
	old_path = dirs[active_pane]->path;
	if (strlen(path) < strlen(old_path)) {
		UI_DirectoryPaneSearch(new_pane, PathBaseName(old_path));
	}

	free(path);

	if (new_pane != NULL) {
		VFS_CloseDir(dirs[active_pane]);
		UI_PaneHide(pane);
		// TODO UI_DirectoryPaneFree(pane);
		panes[active_pane] = new_pane;
		dirs[active_pane] = new_dir;
		UI_PaneShow(new_pane);

		SwitchToPane(active_pane);
		UI_TextInputClear(&search_pane.input);
	}
}

static void OpenEntry(void)
{
	struct directory_pane *pane = panes[active_pane];
	enum file_type typ;
	int selected;

	typ = UI_DirectoryPaneEntryType(pane);

	// Change directory?
	if (typ == FILE_TYPE_WAD || typ == FILE_TYPE_DIR) {
		NavigateNew();
		return;
	}

	selected = UI_DirectoryPaneSelected(pane);
	if (selected < 0) {
		return;
	}

	PerformView(pane->dir, &pane->dir->entries[selected]);
}

static const struct action *actions[] = {
	&copy_action,
	&copy_noconv_action,
	&make_wad_action,
	&make_wad_noconv_action,
};

static void HandleKeypress(void *pane, int key)
{
	int i;

	for (i = 0; i < arrlen(actions); i++) {
		if (key == actions[i]->key
		 || key == CTRL_(actions[i]->ctrl_key)) {
			actions[i]->callback(panes[active_pane],
			                     panes[!active_pane]);
			return;
		}
	}

	switch (key) {
	case KEY_LEFT:
		SwitchToPane(0);
		break;
	case KEY_RIGHT:
		SwitchToPane(1);
		break;
	case KEY_RESIZE:
		SetWindowSizes();
		break;
	case CTRL_('D'):  // ^D = display; toggle UI
		cmdr_mode = !cmdr_mode;
		SetWindowSizes();
		break;
	case CTRL_('L'):  // ^L = redraw whole screen
		clearok(stdscr, TRUE);
		wrefresh(stdscr);
		break;
	case CTRL_('R'):  // ^R = reload dir
		VFS_Refresh(dirs[active_pane]);
		break;
	case CTRL_('N'):  // ^N = search again
		UI_DirectoryPaneSearchAgain(panes[active_pane],
		                            search_pane.input.input);
		break;
	case '\r':
		OpenEntry();
		break;
	case '\t':
		SwitchToPane(!active_pane);
		break;
	case 27:
		UI_ExitMainLoop();
		break;
	default:
		UI_PaneKeypress(panes[active_pane], key);
		break;
	}
}

static void DrawInfoPane(void *p)
{
	struct pane *pane = p;
	int idx = UI_DirectoryPaneSelected(panes[active_pane]);

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	UI_DrawWindowBox(pane->window);
	mvwaddstr(pane->window, 0, 2, " Info ");

	if (idx < 0) {
		return;
	}
	if (dirs[active_pane]->entries[idx].type == FILE_TYPE_LUMP) {
		struct wad_file *wf = VFS_WadFile(dirs[active_pane]);
		const struct lump_type *lt = LI_IdentifyLump(wf, idx);
		UI_PrintMultilineString(pane->window, 1, 2,
		    LI_DescribeLump(lt, wf, idx));
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
	dirs[0] = VFS_OpenDir(start_path1);
	if (dirs[0] == NULL) {
		Shutdown();
		fprintf(stderr, "Failed to open '%s'.\n", start_path1);
		exit(-1);
	}
	panes[0] = UI_NewDirectoryPane(pane_windows[0], dirs[0]);
	UI_PaneShow(panes[0]);

	if (dirs[0]->type == FILE_TYPE_WAD
	 && !strcmp(start_path1, start_path2)) {
		Shutdown();
		fprintf(stderr, "Can't open the same WAD in both panes.\n");
		exit(-1);
	}
	pane_windows[1] = newwin(24, 27, 1, 53);
	dirs[1] = VFS_OpenDir(start_path2);
	if (dirs[1] == NULL) {
		Shutdown();
		fprintf(stderr, "Failed to open '%s'.\n", start_path2);
		exit(-1);
	}
	panes[1] = UI_NewDirectoryPane(pane_windows[1], dirs[1]);
	UI_PaneShow(panes[1]);

	SwitchToPane(0);

	SetWindowSizes();
	UI_RunMainLoop();

	Shutdown();
}
