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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "colors.h"
#include "common.h"
#include "dialog.h"
#include "strings.h"
#include "ui.h"
#include "directory_pane.h"

static void DrawEntry(WINDOW *win, int idx, void *data)
{
	struct directory_pane *dp = data;
	const struct directory_entry *ent;
	static char buf[128];
	unsigned int w;
	int prefix = ' ';
	char size[10] = "";
	bool shorter;

	w = getmaxx(win) - 2;
	if (w > sizeof(buf)) {
		w = sizeof(buf);
	}
	shorter = w < 16;

	if (idx == LIST_PANE_END_MARKER) {
		if (dp->dir->num_entries == 0) {
			mvwaddstr(win, 0, 0, " (empty)");
		}
		return;
	}

	if (idx == 0) {
		char *parent = PathDirName(dp->dir->path);
		prefix = '^';
		snprintf(buf, w, " Parent (%s) ", PathBaseName(parent));
		free(parent);
		wattron(win, COLOR_PAIR(PAIR_DIRECTORY));
	} else {
		ent = &dp->dir->entries[idx - 1];
		switch (ent->type) {
			case FILE_TYPE_DIR:
				wattron(win, A_BOLD);
				prefix = '/';
				break;
			case FILE_TYPE_WAD:
				wattron(win, COLOR_PAIR(PAIR_WAD_FILE));
				break;
			default:
				wattron(win, COLOR_PAIR(PAIR_WHITE_BLACK));
				break;
		}

		// Show insert point for where we'll import into the WAD:
		if (!dp->pane.active && idx == dp->pane.selected
		 && dp->dir->type != FILE_TYPE_DIR) {
			if ((termattrs() & A_UNDERLINE) != 0) {
				wattron(win, A_UNDERLINE);
			} else {
				prefix = '_';
			}
		}

		// We only show size for lumps (like NWT); for files it
		// is too cluttered (plus filenames can be much longer)
		if (ent->type == FILE_TYPE_LUMP) {
			VFS_DescribeSize(ent, size, shorter);
		}
		snprintf(buf, w, "%-100s", ent->name);
	}
	if (dp->pane.active && idx == dp->pane.selected) {
		wattron(win, A_REVERSE);
	} else if (idx > 0 &&
	           VFS_SetHas(&dp->tagged,
	                      dp->dir->entries[idx - 1].serial_no)) {
		wattron(win, COLOR_PAIR(PAIR_TAGGED));
	}
	mvwaddch(win, 0, 0, prefix);
	waddstr(win, buf);
	if (idx == 0) {
		mvwaddch(win, 0, 0, ACS_LLCORNER);
		mvwaddch(win, 0, 1, ACS_HLINE);
	} else {
		mvwaddstr(win, 0, w - strlen(size) - 1, " ");
		waddstr(win, size);
		waddstr(win, " ");
	}
	wattroff(win, A_UNDERLINE);
	wattroff(win, A_REVERSE);
	wattroff(win, A_BOLD);
	wattroff(win, COLOR_PAIR(PAIR_WHITE_BLACK));
	wattroff(win, COLOR_PAIR(PAIR_DIRECTORY));
	wattroff(win, COLOR_PAIR(PAIR_WAD_FILE));
	wattroff(win, COLOR_PAIR(PAIR_TAGGED));
}

static unsigned int NumEntries(void *data)
{
	struct directory_pane *dp = data;
	return dp->dir->num_entries + 1;
}

void UI_DirectoryPaneReselect(struct directory_pane *p)
{
	if (p->pane.selected > p->dir->num_entries) {
		p->pane.selected = p->dir->num_entries;
	}
}

void UI_DirectoryPaneSelectEntry(struct directory_pane *p,
                                 struct directory_entry *ent)
{
	unsigned int idx = ent - p->dir->entries;
	if (idx >= p->dir->num_entries) {
		return;
	}
	UI_ListPaneSelect(&p->pane, idx + 1);
}

void UI_DirectoryPaneSelectByName(struct directory_pane *p, const char *name)
{
	struct directory_entry *entry = VFS_EntryByName(p->dir, name);
	if (entry != NULL) {
		UI_DirectoryPaneSelectEntry(p, entry);
	}
}

void UI_DirectoryPaneSelectBySerial(struct directory_pane *p,
                                    uint64_t serial_no)
{
	struct directory_entry *entry = VFS_EntryBySerial(p->dir, serial_no);
	if (entry != NULL) {
		UI_DirectoryPaneSelectEntry(p, entry);
	}
}

static bool PrefixSearch(struct directory_pane *dp, const char *needle,
                         int start_index)
{
	size_t needle_len = strlen(needle);
	const struct directory_entry *ent;
	int i;

	for (i = start_index; i < dp->dir->num_entries; i++) {
		ent = &dp->dir->entries[i];
		if (!strncasecmp(ent->name, needle, needle_len)) {
			dp->pane.selected = i + 1;
			dp->pane.window_offset = dp->pane.selected >= 10 ?
			    dp->pane.selected - 10 : 0;
			return true;
		}
	}

	return false;
}

static bool SubstringSearch(struct directory_pane *dp, const char *needle,
                            int start_index)
{
	size_t haystack_len, needle_len = strlen(needle);
	const struct directory_entry *ent;
	int i, j;

	for (i = start_index; i < dp->dir->num_entries; i++) {
		ent = &dp->dir->entries[i];
		haystack_len = strlen(ent->name);
		if (haystack_len < needle_len) {
			continue;
		}
		for (j = 0; j < haystack_len - needle_len + 1; j++) {
			if (!strncasecmp(&ent->name[j], needle, needle_len)) {
				dp->pane.selected = i + 1;
				dp->pane.window_offset = dp->pane.selected >= 10 ?
				    dp->pane.selected - 10 : 0;
				return true;
			}
		}
	}

	return false;
}

void UI_DirectoryPaneSearch(void *p, const char *needle)
{
	struct directory_pane *dp = p;

	if (strlen(needle) == 0) {
		return;
	}

	if (!strcmp(needle, "..")) {
		dp->pane.selected = 0;
		dp->pane.window_offset = 0;
		return;
	}

	// Check for prefix first, so user can type entire lump name.
	if (!PrefixSearch(dp, needle, 0)) {
		// If nothing found, try a substring match.
		(void) SubstringSearch(dp, needle, 0);
	}
}

void UI_DirectoryPaneSearchAgain(void *p, const char *needle)
{
	struct directory_pane *dp = p;
	int start_index = dp->pane.selected;

	if (strlen(needle) == 0 || !strcmp(needle, "..")) {
		return;
	}

	// When searching again, we only do substring matches.
	if (!SubstringSearch(dp, needle, start_index)) {
		(void) SubstringSearch(dp, needle, 0);
	}
}

int UI_DirectoryPaneSelected(struct directory_pane *p)
{
	return UI_ListPaneSelected(&p->pane) - 1;
}

struct file_set *UI_DirectoryPaneTagged(struct directory_pane *p)
{
	if (p->tagged.num_entries > 0) {
		return &p->tagged;
	} else {
		static struct file_set result;
		int selected = UI_DirectoryPaneSelected(p);
		if (selected >= 0) {
			// Some devious pointer magic here.
			result.entries = &p->dir->entries[selected].serial_no;
			result.num_entries = 1;
		} else {
			result.entries = NULL;
			result.num_entries = 0;
		}
		return &result;
	}
}

void UI_DirectoryPaneSetTagged(struct directory_pane *p, struct file_set *set)
{
	struct directory_entry *dirent;
	int idx = 0;

	VFS_CopySet(&p->tagged, set);

	// Jump to first in the set.
	dirent = VFS_IterateSet(p->dir, &p->tagged, &idx);
	if (dirent != NULL) {
		UI_DirectoryPaneSelectBySerial(p, dirent->serial_no);
	}
}

static const struct list_pane_funcs directory_pane_funcs = {
	DrawEntry,
	NumEntries,
};

enum file_type UI_DirectoryPaneEntryType(struct directory_pane *p)
{
	int selected = UI_DirectoryPaneSelected(p);

	if (selected < 0) {
		return FILE_TYPE_DIR;
	}
	return p->dir->entries[selected].type;
}

// Get path to currently selected entry.
char *UI_DirectoryPaneEntryPath(struct directory_pane *p)
{
	int selected = UI_DirectoryPaneSelected(p);

	if (selected < 0) {
		return PathDirName(p->dir->path);
	} else {
		return VFS_EntryPath(p->dir, &p->dir->entries[selected]);
	}
}

static void DrawPane(void *p)
{
	struct directory_pane *dp = p;
	WINDOW *win = dp->pane.pane.window;
	int w;
	UI_ListPaneDraw(p);

	if (dp->tagged.num_entries != 0) {
		char buf[16];

		w = getmaxx(win);
		if (w - strlen(dp->pane.title) > 22) {
			snprintf(buf, sizeof(buf), "[%d marked]",
			         (int) dp->tagged.num_entries);
		} else {
			snprintf(buf, sizeof(buf), "[%d]",
			         (int) dp->tagged.num_entries);
		}

		wattron(win, COLOR_PAIR(PAIR_PANE_COLOR));
		mvwaddstr(win, 0, w - strlen(buf) - 3, buf);
		wattroff(win, COLOR_PAIR(PAIR_PANE_COLOR));
	}
}

struct directory_pane *UI_NewDirectoryPane(
	WINDOW *w, struct directory *dir)
{
	struct directory_pane *p;

	p = calloc(1, sizeof(struct directory_pane));
	UI_ListPaneInit(&p->pane, w, &directory_pane_funcs, p);
	p->pane.pane.draw = DrawPane;
	// TODO: Free
	UI_ListPaneSetTitle(&p->pane, PathBaseName(dir->path));
	p->dir = dir;
	// Select first item (assuming there is one):
	UI_ListPaneKeypress(&p->pane, KEY_DOWN);

	return p;
}
