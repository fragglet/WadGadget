#include <curses.h>
#include <string.h>

#include "dir_pane.h"
#include "wad_pane.h"

#define PAIR_PANE_COLOR 1
#define PAIR_HIGHLIGHT  2

#define FILE_PANE_WIDTH 27

static WINDOW *pane_windows[2];
static struct list_pane *panes[2];
static unsigned int active_pane = 0;
static WINDOW *actions_win, *info_win, *search_win, *header_win;

struct list_pane_action common_actions[] =
{
	{"Space", "Mark/unmark"},
	{"F10", "Unmark all"},
	{"", ""},
	{"Tab", "Other pane"},
	{"ESC", "Quit"},
	{NULL, NULL},
};

static unsigned int ScreenColumns(void)
{
	int x, y;
	getmaxyx(stdscr, y, x);
	return x;
}

static unsigned int ScreenLines(void)
{
	int x, y;
	getmaxyx(stdscr, y, x);
	return y;
}

static void ShowHeader()
{
	wbkgdset(header_win, COLOR_PAIR(PAIR_HIGHLIGHT));
	werase(header_win);
	mvwaddstr(header_win, 0, 1, "= WadGadget for Doom, Heretic, Hexen, "
	                "Strife, Chex Quest and the rest =");
	wrefresh(header_win);
}

static void ShowInfoWindow()
{
	wbkgdset(info_win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(info_win);
	box(info_win, 0, 0);
	mvwaddstr(info_win, 0, 2, " Info ");

	mvwaddstr(info_win, 1, 2, "TITLEPIC  123 bytes");
	mvwaddstr(info_win, 2, 2, "Dimensions: 320x200");
	wrefresh(info_win);
}

static void ShowAction(int y, const struct list_pane_action *action)
{
	char *desc;

	if (strlen(action->key) == 0) {
		return;
	}
	wattron(actions_win, A_BOLD);
	mvwaddstr(actions_win, y, 2, action->key);
	wattroff(actions_win, A_BOLD);
	waddstr(actions_win, " - ");
	desc = action->description;
	if (action->description[0] == '>') {
		if (active_pane == 1) {
			wattron(actions_win, A_BOLD);
			waddstr(actions_win, "<<< ");
			wattroff(actions_win, A_BOLD);
		}
		desc += 2;
	}
	waddstr(actions_win, desc);
	if (action->description[0] == '>') {
		if (active_pane == 0) {
			wattron(actions_win, A_BOLD);
			waddstr(actions_win, " >>>");
			wattroff(actions_win, A_BOLD);
		}
	}
}

static void ShowActions(void)
{
	const struct list_pane_action *actions;
	int i, y;

	wbkgdset(actions_win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(actions_win);
	box(actions_win, 0, 0);
	mvwaddstr(actions_win, 0, 2, " Actions ");

	actions = UI_ListPaneActions(panes[active_pane], panes[!active_pane]);

	y = 1;
	for (i = 0; actions != NULL && actions[i].key != NULL; i++) {
		ShowAction(y, &actions[i]);
		y++;
	}
	for (i = 0; common_actions[i].key != NULL; i++) {
		ShowAction(y, &common_actions[i]);
		y++;
	}
	wrefresh(actions_win);
}

static void ShowSearchWindow(void)
{
	wbkgdset(search_win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(search_win);
	box(search_win, 0, 0);
	mvwaddstr(search_win, 1, 2, "Search: ");
	mvwaddstr(search_win, 2, 2, "");
	wrefresh(search_win);
}

static void SetWindowSizes(void)
{
	int lines = ScreenLines(), columns = ScreenColumns();
	int pane_width = (columns > 80) ? (FILE_PANE_WIDTH * columns) / 80
	                              : FILE_PANE_WIDTH;
	int middle_width = (columns > 80 ? columns : 80) - pane_width * 2;
	wresize(header_win, 1, columns);
	wresize(info_win, 5, middle_width);
	mvwin(info_win, 1, pane_width);
	wresize(search_win, 4, middle_width);
	mvwin(search_win, lines - 4, pane_width);
	
	wresize(actions_win, 14, middle_width);
	mvwin(actions_win, 6, pane_width);
	wresize(pane_windows[0], lines - 1, pane_width);
	mvwin(pane_windows[0], 1, 0);
	wresize(pane_windows[1], lines - 1, pane_width);
	mvwin(pane_windows[1], 1, columns - pane_width);

	erase();
	refresh();
}

int main(int argc, char *argv[])
{
	WINDOW *pane;

	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);

	init_pair(PAIR_PANE_COLOR, COLOR_WHITE, COLOR_BLUE);
	init_pair(PAIR_HIGHLIGHT, COLOR_BLACK, COLOR_CYAN);

	refresh();

	WINDOW *header;
	header_win = newwin(1, 80, 0, 0);
	info_win = newwin(5, 80 - (FILE_PANE_WIDTH * 2),
	                  1, FILE_PANE_WIDTH);
	search_win = newwin(4, 80 - (FILE_PANE_WIDTH * 2),
	                    20, FILE_PANE_WIDTH);
	actions_win = newwin(14, 80 - (FILE_PANE_WIDTH * 2),
	                     6, FILE_PANE_WIDTH);
	pane_windows[0] = newwin(
		ScreenLines() - 1, FILE_PANE_WIDTH, 1, 0);
	panes[0] = UI_NewWadPane(pane_windows[0], W_OpenFile("doom2.wad"));
	pane_windows[1] = newwin(ScreenLines() - 1, FILE_PANE_WIDTH,
		1, 80 - FILE_PANE_WIDTH);
	panes[1] = UI_NewDirectoryPane(pane_windows[1],
		DIR_ReadDirectory("/home/fraggle"));
	UI_ListPaneActive(panes[active_pane], 1);

	SetWindowSizes();

	for (;;) {
		int key;

		ShowHeader();
		ShowInfoWindow();
		ShowActions();
		ShowSearchWindow();
		UI_DrawListPane(panes[0]);
		UI_DrawListPane(panes[1]);

		key = getch();
		switch (key) {
		case KEY_LEFT:
			active_pane = 0;
			UI_ListPaneActive(panes[0], 1);
			UI_ListPaneActive(panes[1], 0);
			break;
		case KEY_RIGHT:
			active_pane = 1;
			UI_ListPaneActive(panes[0], 0);
			UI_ListPaneActive(panes[1], 1);
			break;
		case KEY_RESIZE:
			SetWindowSizes();
			break;
		case '\t':
			UI_ListPaneActive(panes[active_pane], 0);
			active_pane = !active_pane;
			UI_ListPaneActive(panes[active_pane], 1);
			break;
		default:
			UI_ListPaneInput(panes[active_pane], key);
			break;
		}
	}
	endwin();
}
