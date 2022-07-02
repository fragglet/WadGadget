#include <curses.h>

#define PAIR_PANE_COLOR 1
#define PAIR_HIGHLIGHT  2

struct {
	char *key;
	char *desc;
} hotkeys[] = {
	{"F2", "Right"},
	{"F3", "Open"},
	{"F4", "Edit"},
	{"F5", "Export >>>"},
	{"F6", "Rename"},
	{"F8", "Delete"},
	{"F9", "New lump"},
	{NULL, NULL},
};

void show_header()
{
	WINDOW *header;

	header = newwin(1, 78, 0, 0);
	wbkgdset(header, COLOR_PAIR(PAIR_HIGHLIGHT));
	werase(header);
	mvwaddstr(header, 0, 1, "= BlueWadTool for Doom, Heretic, Hexen, "
	                "Strife, Chex Quest and the rest =");
	wrefresh(header);
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
	mvaddstr(41, 0, "Search: ");
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

	pane = newwin(40, 39, 1, 0);
	wbkgdset(pane, COLOR_PAIR(PAIR_PANE_COLOR));
	wattron(pane, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane);
	box(pane, 0, 0);
	mvwaddstr(pane, 0, 3, " doom2.wad ");
	wrefresh(pane);

	pane = newwin(40, 39, 1, 39);
	wbkgdset(pane, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane);
	box(pane, 0, 0);
	wattron(pane, COLOR_PAIR(PAIR_HIGHLIGHT));
	mvwaddstr(pane, 0, 3, " /home/fraggle ");
	wrefresh(pane);
	show_accelerators();
	getch();


	endwin();
}
