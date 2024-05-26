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

#define KB(x) (x * 1000ULL)
#define MB(x) (KB(x) * 1000ULL)
#define GB(x) (MB(x) * 1000ULL)
#define TB(x) (GB(x) * 1000ULL)

static void SummarizeSize(int64_t len, char buf[10], bool shorter)
{
	int64_t adj_len = len * (shorter ? 100 : 1);

	if (adj_len < 0) {
		strncpy(buf, "", 10);
	} else if (adj_len < KB(100)) {  // up to 99999
		snprintf(buf, 10, " %d", (int) len);
	} else if (adj_len < MB(10)) {  // up to 9999K
		snprintf(buf, 10, " %dK", (int) (len / KB(1)));
	} else if (adj_len < GB(10)) {  // up to 9999M
		snprintf(buf, 10, " %dM", (int) (len / MB(1)));
	} else if (adj_len < TB(10)) {  // up to 9999G
		snprintf(buf, 10, " %dG", (int) (len / GB(1)));
	} else {
		snprintf(buf, 10, " big!");
	}
}

static void DrawEntry(WINDOW *win, int idx, void *data)
{
	struct directory_pane *dp = data;
	const struct directory_entry *ent;
	static char buf[128];
	unsigned int w, h;
	char size[10] = "";
	bool shorter;

	getmaxyx(win, h, w);
	w -= 2; h = h;
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
		snprintf(buf, w, "^  Parent (%s)", PathBaseName(parent));
		free(parent);
		wattron(win, COLOR_PAIR(PAIR_DIRECTORY));
	} else {
		char prefix = ' ';

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
		SummarizeSize(ent->size, size, shorter);
		snprintf(buf, w, "%c%-100s", prefix, ent->name);
	}
	if (dp->pane.active && idx == dp->pane.selected) {
		wattron(win, A_REVERSE);
	} else if (idx > 0 &&
	           VFS_SetHas(&dp->tagged,
	                      dp->dir->entries[idx - 1].serial_no)) {
		wattron(win, COLOR_PAIR(PAIR_TAGGED));
	}
	mvwaddstr(win, 0, 0, buf);
	waddstr(win, " ");
	if (idx == 0) {
		mvwaddch(win, 0, 0, ACS_LLCORNER);
		mvwaddch(win, 0, 1, ACS_HLINE);
	}
	mvwaddstr(win, 0, w - strlen(size), size);
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

void UI_DirectoryPaneSelectByName(struct directory_pane *p, const char *name)
{
	struct directory_entry *entry = VFS_EntryByName(p->dir, name);
	if (entry != NULL) {
		UI_ListPaneSelect(&p->pane, entry - p->dir->entries + 1);
	}
}

void UI_DirectoryPaneSelectBySerial(struct directory_pane *p,
                                    uint64_t serial_no)
{
	struct directory_entry *entry = VFS_EntryBySerial(p->dir, serial_no);
	if (entry != NULL) {
		UI_ListPaneSelect(&p->pane, entry - p->dir->entries + 1);
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

struct directory_pane *UI_NewDirectoryPane(
	WINDOW *w, struct directory *dir)
{
	struct directory_pane *p;

	p = calloc(1, sizeof(struct directory_pane));
	UI_ListPaneInit(&p->pane, w, &directory_pane_funcs, p);
	// TODO: Free
	UI_ListPaneSetTitle(&p->pane, PathBaseName(dir->path));
	p->dir = dir;
	// Select first item (assuming there is one):
	UI_ListPaneKeypress(&p->pane, KEY_DOWN);

	return p;
}
