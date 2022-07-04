#include <curses.h>
#include <string.h>

#include "dir_pane.h"
#include "wad_pane.h"

#define PAIR_PANE_COLOR 1
#define PAIR_HIGHLIGHT  2

#define FILE_PANE_WIDTH 27

struct {
	char *key;
	char *desc;
} hotkeys[] = {
	{"F1", "Hexdump"},
	{"F3", "Open"},
	{"F4", "Edit"},
	{"F5", "Export >>>"},
	{"F6", "Rename"},
	{"F8", "Delete"},
	{"F9", "New lump"},
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

void show_middle_accelerators()
{
	WINDOW *win;
	int i;

	win = newwin(14, 80 - (FILE_PANE_WIDTH * 2),
	             6, FILE_PANE_WIDTH);
	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	box(win, 0, 0);
	mvwaddstr(win, 0, 2, " Actions ");

	for (i = 0; hotkeys[i].key != NULL; i++) {
		if (strlen(hotkeys[i].key) == 0) {
			continue;
		}
		wattron(win, A_BOLD);
		mvwaddstr(win, 1+i, 2, hotkeys[i].key);
		wattroff(win, A_BOLD);
		waddstr(win, " - ");
		waddstr(win, hotkeys[i].desc);
	}
	wrefresh(win);
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

void show_accelerators()
{
	int i;
	mvaddstr(42, 0, "");
	for (i = 0; hotkeys[i].key != NULL; i++) {
		addstr(" ");
		attron(A_BOLD);
		addstr(hotkeys[i].key);
		attroff(A_BOLD);
		attron(COLOR_PAIR(PAIR_HIGHLIGHT));
		addstr(hotkeys[i].desc);
		attroff(COLOR_PAIR(PAIR_HIGHLIGHT));
	}
}

int main(int argc, char *argv[])
{
	struct list_pane *pane1, *pane2;
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

	pane1 = UI_NewWadPane(
		newwin(23, FILE_PANE_WIDTH, 1, 0),
		W_OpenFile("doom2.wad"));
	UI_DrawListPane(pane1);

	pane2 = UI_NewDirectoryPane(
		newwin(23, FILE_PANE_WIDTH, 1, 80 - FILE_PANE_WIDTH),
		"/home/fraggle");
	UI_DrawListPane(pane2);

	show_info_box();
	show_middle_accelerators();
	show_search_box();

	for (;;) {
		int key = getch();
		UI_ListPaneInput(pane1, key);
		UI_DrawListPane(pane1);
		UI_DrawListPane(pane2);
	}
	endwin();
}
