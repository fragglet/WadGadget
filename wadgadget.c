#include <curses.h>
#include <string.h>

#include "common.h"
#include "dir_pane.h"
#include "wad_pane.h"
#include "ui.h"

#define FILE_PANE_WIDTH 27

static struct actions_pane actions_pane;
static struct pane header_pane, info_pane, search_pane;
static WINDOW *pane_windows[2];
static struct list_pane *panes[2];
static void *pane_data[2];
static unsigned int active_pane = 0;
static int main_loop_exited = 0;

static unsigned int ScreenColumns(void)
{
	int x, y;
	getmaxyx(stdscr, y, x);
	y = y;
	return x;
}

static unsigned int ScreenLines(void)
{
	int x, y;
	getmaxyx(stdscr, y, x);
	x = x;
	return y;
}

static void SetWindowSizes(void)
{
	int lines = ScreenLines(), columns = ScreenColumns();
	int pane_width = max(columns, 80) * FILE_PANE_WIDTH / 80;
	int middle_width = max(columns, 80) - pane_width * 2;
	wresize(header_pane.window, 1, columns);
	wresize(info_pane.window, 5, middle_width);
	mvwin(info_pane.window, 1, pane_width);
	wresize(search_pane.window, 4, middle_width);
	mvwin(search_pane.window, lines - 4, pane_width);
	
	wresize(actions_pane.pane.window, 14, middle_width);
	mvwin(actions_pane.pane.window, 6, pane_width);
	wresize(pane_windows[0], lines - 1, pane_width);
	mvwin(pane_windows[0], 1, 0);
	wresize(pane_windows[1], lines - 1, pane_width);
	mvwin(pane_windows[1], 1, columns - pane_width);

	erase();
	refresh();
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
		UI_PaneActive(panes[active_pane], 1);
		UI_PaneShow(new_pane);
	}
}

static void HandleKeypress(int key)
{
	switch (key) {
	case KEY_LEFT:
		active_pane = 0;
		UI_PaneActive(panes[0], 1);
		UI_PaneActive(panes[1], 0);
		break;
	case KEY_RIGHT:
		active_pane = 1;
		UI_PaneActive(panes[0], 0);
		UI_PaneActive(panes[1], 1);
		break;
	case KEY_RESIZE:
		SetWindowSizes();
		break;
	case '\r':
		NavigateNew();
		break;
	case '\t':
		UI_PaneActive(panes[active_pane], 0);
		active_pane = !active_pane;
		UI_PaneActive(panes[active_pane], 1);
		break;
	case 27:
		main_loop_exited = 1;
		break;
	default:
		UI_PaneKeypress(panes[active_pane], key);
		break;
	}
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

	init_pair(PAIR_PANE_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(PAIR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);
	init_pair(PAIR_DIRECTORY, COLOR_CYAN, COLOR_BLACK);
	init_pair(PAIR_WAD_FILE, COLOR_RED, COLOR_BLACK);

	refresh();

	UI_InitHeaderPane(&header_pane, newwin(1, 80, 0, 0));
	UI_PaneShow(&header_pane);
	UI_InitInfoPane(
		&info_pane,
		newwin(5, 80 - (FILE_PANE_WIDTH * 2),
		       1, FILE_PANE_WIDTH));
	UI_PaneShow(&info_pane);
	UI_InitSearchPane(
		&search_pane,
		newwin(4, 80 - (FILE_PANE_WIDTH * 2),
		       20, FILE_PANE_WIDTH));
	UI_PaneShow(&search_pane);
	UI_ActionsPaneInit(
		&actions_pane,
		newwin(14, 80 - (FILE_PANE_WIDTH * 2),
		       6, FILE_PANE_WIDTH));
	UI_PaneShow(&actions_pane);
	pane_windows[0] = newwin(
		ScreenLines() - 1, FILE_PANE_WIDTH, 1, 0);
	pane_data[0] = W_OpenFile("doom2.wad");
	panes[0] = UI_NewWadPane(pane_windows[0], pane_data[0]);
	UI_PaneShow(panes[0]);
	pane_windows[1] = newwin(ScreenLines() - 1, FILE_PANE_WIDTH,
		1, 80 - FILE_PANE_WIDTH);
	pane_data[1] = DIR_ReadDirectory("/home/fraggle");
	panes[1] = UI_NewDirectoryPane(pane_windows[1], pane_data[1]);
	UI_PaneShow(panes[1]);
	UI_PaneActive(panes[active_pane], 1);

	SetWindowSizes();

	while (!main_loop_exited) {
		int key;

		actions_pane.actions = UI_ListPaneActions(
			panes[active_pane], panes[!active_pane]);
		UI_DrawAllPanes();

		key = getch();
		HandleKeypress(key);
	}
	clear();
	refresh();
	endwin();
}
