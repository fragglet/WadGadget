#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "common.h"
#include "dialog.h"
#include "dir_pane.h"
#include "export.h"
#include "import.h"
#include "lump_info.h"
#include "strings.h"
#include "wad_pane.h"
#include "ui.h"

#define FILE_PANE_WIDTH 27

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

static struct actions_pane actions_pane;
static struct pane header_pane, info_pane;
static struct search_pane search_pane;
static WINDOW *pane_windows[2];
static struct list_pane *panes[2];
static void *pane_data[2];
static unsigned int active_pane = 0;

static void SetWindowSizes(void)
{
	int pane_width, middle_width;
	int lines, columns;
	getmaxyx(stdscr, lines, columns);
	pane_width = max(columns, 80) * FILE_PANE_WIDTH / 80;
	middle_width = max(columns, 80) - pane_width * 2;

	wresize(header_pane.window, 1, columns);
	wresize(info_pane.window, 5, middle_width);
	mvwin(info_pane.window, 1, pane_width);
	wresize(search_pane.pane.window, 3, middle_width);
	mvwin(search_pane.pane.window, lines - 3,
	      columns - middle_width - pane_width);
	
	wresize(actions_pane.pane.window, 14, middle_width);
	mvwin(actions_pane.pane.window, 6, pane_width);
	wresize(pane_windows[0], lines - 1, pane_width);
	mvwin(pane_windows[0], 1, 0);
	wresize(pane_windows[1], lines - 1, pane_width);
	mvwin(pane_windows[1], 1, columns - pane_width);

	erase();
	refresh();
}

static void SwitchToPane(unsigned int pane)
{
	panes[active_pane]->active = 0;
	active_pane = pane;
	UI_RaisePaneToTop(panes[pane]);
	panes[active_pane]->active = 1;
	// The search pane always sits on top of the stack.
	// intercept keypresses:
	UI_RaisePaneToTop(&search_pane);

	actions_pane.left_to_right = active_pane == 0;
	actions_pane.actions = UI_ListPaneActions(
		panes[active_pane], panes[!active_pane]);
}

static void NavigateNew(void)
{
	struct list_pane *pane = panes[active_pane];
	struct list_pane *new_pane = NULL;
	void *new_data;
	enum blob_type bt;
	const char *path;
	bt = UI_ListPaneEntryType(pane, pane->selected);
	path = UI_ListPaneEntryPath(pane, pane->selected);

	switch (bt) {
	case BLOB_TYPE_DIR:
		new_data = DIR_ReadDirectory(path);
		new_pane = UI_NewDirectoryPane(
			pane_windows[active_pane], new_data);
		break;

	case BLOB_TYPE_WAD:
		new_data = W_OpenFile(path);
		new_pane = UI_NewWadPane(pane_windows[active_pane], new_data);
		break;

	default:
		// TODO: Do something else, like display file contents.
		break;
	}

	if (new_pane != NULL) {
		BL_FreeList(pane_data[active_pane]);
		UI_PaneHide(pane);
		UI_ListPaneFree(pane);
		panes[active_pane] = new_pane;
		pane_data[active_pane] = new_data;
		UI_PaneShow(new_pane);

		SwitchToPane(active_pane);
	}
}

static void PerformCopy(void)
{
	struct list_pane *from, *to;
	from = panes[active_pane]; to = panes[!active_pane];

	if (to->type == PANE_TYPE_DIR) {
		PerformExport(from->blob_list, UI_ListPaneSelected(from),
		              (struct directory_listing *) to->blob_list);
		return;
	}

	if (to->type == PANE_TYPE_WAD) {
		PerformImport(from->blob_list, UI_ListPaneSelected(from),
		              (struct wad_file *) to->blob_list,
		              UI_ListPaneSelected(to) + 1);
		return;
	}

	UI_ConfirmDialogBox("Sorry", "This isn't implemented yet.");
}

static void HandleKeypress(void *pane, int key)
{
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
	case KEY_F(5):
		PerformCopy();
		break;
	case '\r':
		NavigateNew();
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

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	box(pane->window, 0, 0);
	mvwaddstr(pane->window, 0, 2, " Info ");

       if (panes[active_pane]->type == PANE_TYPE_WAD
        && panes[active_pane]->selected > 0) {
		unsigned int lump_index = panes[active_pane]->selected - 1;
		UI_PrintMultilineString(pane->window, 1, 2,
		    GetLumpDescription(pane_data[active_pane], lump_index));
       }
}

static void DrawSearchPane(void *pane)
{
	struct search_pane *p = pane;
	WINDOW *win = p->pane.window;

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 0, 2, " Search ");
	UI_TextInputDraw(&p->input);
}

static void SearchPaneKeypress(void *pane, int key)
{
	struct search_pane *p = pane;
	// Space key triggers mark, does not go to search input.
	if (key != ' ' && UI_TextInputKeypress(&p->input, key)) {
		UI_ListPaneSearch(panes[active_pane], p->input.input);
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
	UI_TextInputInit(&search_pane.input, win, 1, 20);
}

int main(int argc, char *argv[])
{
	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	init_pair(PAIR_WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_PANE_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(PAIR_HEADER, COLOR_BLACK, COLOR_CYAN);
	init_pair(PAIR_DIRECTORY, COLOR_CYAN, COLOR_BLACK);
	init_pair(PAIR_WAD_FILE, COLOR_RED, COLOR_BLACK);
	init_pair(PAIR_DIALOG_BOX, COLOR_WHITE, COLOR_MAGENTA);
	init_pair(PAIR_TAGGED, COLOR_WHITE, COLOR_RED);

	refresh();

	// The hard-coded window sizes and positions here get reset
	// when SetWindowSizes() is called below.
	UI_InitHeaderPane(&header_pane, newwin(1, 80, 0, 0));
	UI_PaneShow(&header_pane);

	InitInfoPane(newwin(5, 26, 1, 27));
	UI_PaneShow(&info_pane);

	InitSearchPane(newwin(4, 26, 20, 27));
	UI_PaneShow(&search_pane);

	UI_ActionsPaneInit(&actions_pane, newwin(14, 26, 6, 27));
	UI_PaneShow(&actions_pane);

	pane_windows[0] = newwin(24, 27, 1, 0);
	pane_data[0] = W_OpenFile("doom2.wad");
	panes[0] = UI_NewWadPane(pane_windows[0], pane_data[0]);
	UI_PaneShow(panes[0]);

	pane_windows[1] = newwin(24, 27, 1, 53);
	pane_data[1] = DIR_ReadDirectory(".");
	panes[1] = UI_NewDirectoryPane(pane_windows[1], pane_data[1]);
	UI_PaneShow(panes[1]);

	SwitchToPane(0);

	SetWindowSizes();
	UI_RunMainLoop();

	clear();
	refresh();
	endwin();
}
