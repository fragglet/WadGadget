#include <curses.h>
#include <stdlib.h>
#include <string.h>

#include "actions_pane.h"
#include "colors.h"
#include "common.h"
#include "dialog.h"
#include "directory_pane.h"
#include "export.h"
#include "import.h"
#include "lump_info.h"
#include "strings.h"
#include "ui.h"

#define INFO_PANE_WIDTH 27

#define COLORX_DARKGREY       (COLOR_BLACK + 8)
#define COLORX_BRIGHTBLUE     (COLOR_BLUE + 8)
#define COLORX_BRIGHTGREEN    (COLOR_GREEN + 8)
#define COLORX_BRIGHTCYAN     (COLOR_CYAN + 8)
#define COLORX_BRIGHTRED      (COLOR_RED + 8)
#define COLORX_BRIGHTMAGENTA  (COLOR_MAGENTA + 8)
#define COLORX_BRIGHTYELLOW   (COLOR_YELLOW + 8)
#define COLORX_BRIGHTWHITE    (COLOR_WHITE + 8)

struct palette {
	size_t num_colors;
	struct { int c, r, g, b; } colors[16];
};

// We use the curses init_color() function to set a custom color palette
// that matches the palette from NWT; these values are from the ScreenPal[]
// array in wadview.c.
#define V(x) ((x * 1000) / 63)

static struct palette nwt_palette = {
	16,
	{
		{COLOR_BLACK,          V( 0), V( 0), V( 0)},
		{COLOR_BLUE,           V( 0), V( 0), V(25)},
		{COLOR_GREEN,          V( 0), V(42), V( 0)},
		{COLOR_CYAN,           V( 0), V(42), V(42)},
		{COLOR_RED,            V(42), V( 0), V( 0)},
		{COLOR_MAGENTA,        V(42), V( 0), V(42)},
		{COLOR_YELLOW,         V(42), V(42), V( 0)},
		{COLOR_WHITE,          V(34), V(34), V(34)},

		{COLORX_DARKGREY,      V( 0), V( 0), V(13)},
		{COLORX_BRIGHTBLUE,    V( 0), V( 0), V(55)},
		{COLORX_BRIGHTGREEN,   V( 0), V(34), V(13)},
		{COLORX_BRIGHTCYAN,    V( 0), V(34), V(55)},
		{COLORX_BRIGHTRED,     V(34), V( 0), V(13)},
		{COLORX_BRIGHTMAGENTA, V(34), V( 0), V(55)},
		{COLORX_BRIGHTYELLOW,  V(34), V(34), V(13)},
		{COLORX_BRIGHTWHITE,   V(55), V(55), V(55)},
	},
};

// Old palette we saved and restore on quit.
static struct palette old_palette;

struct search_pane {
	struct pane pane;
	struct text_input_box input;
};

static struct actions_pane actions_pane;
static struct pane header_pane, info_pane;
static struct search_pane search_pane;
static WINDOW *pane_windows[2];
static struct directory_pane *panes[2];
static struct directory *dirs[2];
static unsigned int active_pane = 0;

static void SetWindowSizes(void)
{
	int left_width, middle_width, right_width;
	int lines, columns;
	getmaxyx(stdscr, lines, columns);

	middle_width = INFO_PANE_WIDTH;
	left_width = (max(columns, 80) - middle_width) / 2;
	right_width = max(columns, 80) - left_width - middle_width;

	wresize(header_pane.window, 1, columns);
	wresize(info_pane.window, 5, middle_width);
	mvwin(info_pane.window, 1, left_width);
	wresize(search_pane.pane.window, 3, middle_width);
	mvwin(search_pane.pane.window, lines - 3,
	      left_width);
	
	wresize(actions_pane.pane.window, 14, middle_width);
	mvwin(actions_pane.pane.window, 6, left_width);
	wresize(pane_windows[0], lines - 1, left_width);
	mvwin(pane_windows[0], 1, 0);
	wresize(pane_windows[1], lines - 1, right_width);
	mvwin(pane_windows[1], 1, columns - right_width);

	erase();
	refresh();
}

static void SwitchToPane(unsigned int pane)
{
	panes[active_pane]->pane.active = 0;
	active_pane = pane;
	UI_RaisePaneToTop(panes[pane]);
	panes[active_pane]->pane.active = 1;
	// The search pane always sits on top of the stack.
	// intercept keypresses:
	UI_RaisePaneToTop(&search_pane);

	UI_ActionsPaneSet(&actions_pane, dirs[active_pane]->type,
	                  dirs[!active_pane]->type, active_pane == 0);
}

static void NavigateNew(void)
{
	struct directory_pane *pane = panes[active_pane];
	struct directory_pane *new_pane = NULL;
	struct directory *new_dir;
	enum file_type typ;
	const char *old_path;
	char *path;

	typ = UI_DirectoryPaneEntryType(pane);
	if (typ != FILE_TYPE_WAD && typ != FILE_TYPE_DIR) {
		return;
	}

	path = UI_DirectoryPaneEntryPath(pane);

	// Don't allow the same WAD to be opened twice.
	if (!strcmp(path, dirs[!active_pane]->path)
	 && dirs[!active_pane]->type == FILE_TYPE_WAD) {
		free(path);
		SwitchToPane(!active_pane);
		return;
	}

	new_dir = VFS_OpenDir(path);
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

static void PerformCopy(void)
{
	struct directory *from = dirs[active_pane],
	                 *to = dirs[!active_pane];
	struct file_set result = EMPTY_FILE_SET;

	// When we do an export or import, we create the new files/lumps
	// in the destination, and then switch to the other pane where they
	// are highlighted. The import/export functions both populate a
	// result set that contains the serial numbers of the new files.
	if (to->type == FILE_TYPE_DIR) {
		struct file_set *export_set =
			UI_DirectoryPaneTagged(panes[active_pane]);
		if (export_set->num_entries < 1) {
			UI_MessageBox(
			    "You have not selected anything to export.");
			VFS_FreeSet(&result);
			return;
		}
		if (PerformExport(from, export_set, to, &result)) {
			UI_DirectoryPaneSetTagged(panes[!active_pane], &result);
			SwitchToPane(!active_pane);
		}
		VFS_FreeSet(&result);
		return;
	}
	if (to->type == FILE_TYPE_WAD) {
		struct file_set *import_set =
			UI_DirectoryPaneTagged(panes[active_pane]);
		int to_point = UI_DirectoryPaneSelected(panes[!active_pane]) + 1;
		if (import_set->num_entries < 1) {
			UI_MessageBox(
			    "You have not selected anything to import.");
			VFS_FreeSet(&result);
			return;
		}
		if (PerformImport(from, import_set, to, to_point, &result)) {
			UI_DirectoryPaneSetTagged(panes[!active_pane], &result);
			SwitchToPane(!active_pane);
		}
		VFS_FreeSet(&result);
		return;
	}

	UI_MessageBox("Sorry, this isn't implemented yet.");
}

static char *CreateWadInDir(struct directory *from, struct file_set *from_set,
                            struct directory *to)
{
	struct file_set result = EMPTY_FILE_SET;
	struct directory *newfile;
	char *filename, *filename2;

	filename = UI_TextInputDialogBox(
		"Make new WAD", 64,
		"Enter name for new WAD file:");

	if (VFS_EntryByName(to, filename) != NULL
	 && !UI_ConfirmDialogBox("Confirm Overwrite", "Overwrite existing '%s'?",
	                         filename)) {
		free(filename);
		return NULL;
	}

	filename2 = StringJoin("", to->path, "/", filename, NULL);

	if (!W_CreateFile(filename2)) {
		UI_MessageBox("Failed to create new WAD file.");
		free(filename);
		free(filename2);
		return NULL;
	}
	newfile = VFS_OpenDir(filename2);
	if (newfile == NULL) {
		UI_MessageBox("Failed to open new file after creating.");
		free(filename);
		free(filename2);
		return NULL;
	}

	free(filename2);

	if (!PerformImport(from, from_set, newfile, 0, &result)) {
		free(filename);
		filename = NULL;
	}
	VFS_Refresh(to);
	VFS_FreeSet(&result);
	VFS_CloseDir(newfile);

	return filename;
}

static void CreateWad(void)
{
	struct directory_pane *from_pane, *to_pane;
	struct file_set *import_set;
	char *filename;

	if (panes[active_pane]->dir->type == FILE_TYPE_WAD
	 && panes[!active_pane]->dir->type == FILE_TYPE_DIR) {
		// Export from existing WAD to new WAD
		from_pane = panes[active_pane];
		to_pane = panes[!active_pane];
		if (from_pane->tagged.num_entries == 0) {
			UI_MessageBox("You have not selected any "
			              "lumps to export.");
			return;
		}
	} else if (panes[active_pane]->dir->type == FILE_TYPE_DIR) {
		// Create new WAD and import tagged files.
		from_pane = panes[active_pane];
		to_pane = panes[active_pane];
		if (from_pane->tagged.num_entries == 0
		 && !UI_ConfirmDialogBox("Create WAD",
		                         "Create an empty WAD file?")) {
			return;
		}
	} else {
		return;
	}

	import_set = &from_pane->tagged;
	filename = CreateWadInDir(from_pane->dir, import_set, to_pane->dir);
	if (filename != NULL) {
		UI_DirectoryPaneSearch(to_pane, filename);
		free(filename);
		SwitchToPane(panes[0] == to_pane ? 0 : 1);
	}
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
	case KEY_F(9):
		CreateWad();
		break;
	case ('L' & 0x1f):  // ^L = redraw whole screen
		clearok(stdscr, TRUE);
		wrefresh(stdscr);
		break;
	case ('R' & 0x1f):  // ^R = reload dir
		VFS_Refresh(dirs[active_pane]);
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
	int idx = UI_DirectoryPaneSelected(panes[active_pane]);

	wbkgdset(pane->window, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(pane->window);
	box(pane->window, 0, 0);
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
	UI_TextInputInit(&search_pane.input, win, 1, 20);
}

static void SavePalette(struct palette *p)
{
	short r, g, b;
	int i;

	p->num_colors = 16;
	for (i = 0; i < p->num_colors; i++) {
		p->colors[i].c = i;
		color_content(i, &r, &g, &b);
		p->colors[i].r = r;
		p->colors[i].g = g;
		p->colors[i].b = b;
	}
}

static void SetPalette(struct palette *p)
{
	int i;

	for (i = 0; i < p->num_colors; i++) {
		init_color(p->colors[i].c, p->colors[i].r, p->colors[i].g,
		           p->colors[i].b);
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

	SavePalette(&old_palette);
	SetPalette(&nwt_palette);
	init_pair(PAIR_WHITE_BLACK, COLORX_BRIGHTWHITE, COLOR_BLACK);
	init_pair(PAIR_PANE_COLOR, COLORX_BRIGHTWHITE, COLOR_BLUE);
	init_pair(PAIR_HEADER, COLOR_BLACK, COLORX_BRIGHTCYAN);
	init_pair(PAIR_DIRECTORY, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_WAD_FILE, COLOR_RED, COLOR_BLACK);
	init_pair(PAIR_DIALOG_BOX, COLORX_BRIGHTWHITE, COLOR_MAGENTA);
	init_pair(PAIR_TAGGED, COLORX_BRIGHTWHITE, COLOR_RED);

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
	dirs[0] = VFS_OpenDir(".");
	panes[0] = UI_NewDirectoryPane(pane_windows[0], dirs[0]);
	UI_PaneShow(panes[0]);

	pane_windows[1] = newwin(24, 27, 1, 53);
	dirs[1] = VFS_OpenDir(".");
	panes[1] = UI_NewDirectoryPane(pane_windows[1], dirs[1]);
	UI_PaneShow(panes[1]);

	SwitchToPane(0);

	SetWindowSizes();
	UI_RunMainLoop();

	SetPalette(&old_palette);
	clear();
	refresh();
	endwin();
}
