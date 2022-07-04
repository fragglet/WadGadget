#include <curses.h>
#include <string.h>

#include "dir_pane.h"
#include "wad_pane.h"

#define PAIR_PANE_COLOR 1
#define PAIR_HIGHLIGHT  2

#define FILE_PANE_WIDTH 27

static struct list_pane *panes[2];
static unsigned int active_pane = 0;
static WINDOW *actions_win;

struct list_pane_action common_actions[] =
{
	{"Space", "Mark/unmark"},
	{"F10", "Unmark all"},
	{"", ""},
	{"Tab", "Other pane"},
	{"ESC", "Quit"},
	{NULL, NULL},
};

void show_header()
{
	WINDOW *header;

	header = newwin(1, 80, 0, 0);
	wbkgdset(header, COLOR_PAIR(PAIR_HIGHLIGHT));
	werase(header);
	mvwaddstr(header, 0, 1, "= WadGadget for Doom, Heretic, Hexen, "
	                "Strife, Chex Quest and the rest =");
	wrefresh(header);
}

void show_info_box()
{
	WINDOW *win;

	win = newwin(5, 80 - (FILE_PANE_WIDTH * 2),
	             1, FILE_PANE_WIDTH);
	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 0, 2, " Info ");

	mvwaddstr(win, 1, 2, "TITLEPIC  123 bytes");
	mvwaddstr(win, 2, 2, "Dimensions: 320x200");
	wrefresh(win);
}

static void ShowAction(int y, const struct list_pane_action *action)
{
	if (strlen(action->key) == 0) {
		return;
	}
	wattron(actions_win, A_BOLD);
	mvwaddstr(actions_win, y, 2, action->key);
	wattroff(actions_win, A_BOLD);
	waddstr(actions_win, " - ");
	waddstr(actions_win, action->description);
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

void show_search_box()
{
	WINDOW *win;

	win = newwin(4, 80 - (FILE_PANE_WIDTH * 2),
	             20, FILE_PANE_WIDTH);
	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 1, 2, "Search: ");
	mvwaddstr(win, 2, 2, "");
	wrefresh(win);
}

static unsigned int ScreenLines(void)
{
        int x, y;
        getmaxyx(stdscr, y, x);
        return y;
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
	show_header();

	actions_win = newwin(14, 80 - (FILE_PANE_WIDTH * 2),
	                     6, FILE_PANE_WIDTH);
	panes[0] = UI_NewWadPane(
		newwin(ScreenLines() - 1, FILE_PANE_WIDTH, 1, 0),
		W_OpenFile("doom2.wad"));
	panes[1] = UI_NewDirectoryPane(
		newwin(ScreenLines() - 1, FILE_PANE_WIDTH, 1,
		       80 - FILE_PANE_WIDTH),
		"/home/fraggle");
	UI_ListPaneActive(panes[active_pane], 1);

	show_info_box();
	show_search_box();

	for (;;) {
		int key;
		UI_DrawListPane(panes[0]);
		UI_DrawListPane(panes[1]);
		ShowActions();
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
