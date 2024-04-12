//
// Copyright(C) 2022-2024 Simon Howard
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>

#include "colors.h"
#include "common.h"
#include "dialog.h"
#include "strings.h"
#include "ui.h"
#include "directory_pane.h"

static void SummarizeSize(int64_t len, char buf[10])
{
	if (len < 0) {
		strncpy(buf, "", 10);
	} else if (len < 1000000) {
		snprintf(buf, 10, " %d", (int) len);
	} else if (len < 1000000000) {
		snprintf(buf, 10, " %dK", (int) (len / 1000));
	} else if (len < 1000000000000) {
		snprintf(buf, 10, " %dM", (int) (len / 1000000));
	} else if (len < 1000000000000000) {
		snprintf(buf, 10, " %dG", (int) (len / 1000000000));
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

	getmaxyx(win, h, w);
	w -= 2; h = h;
	if (w > sizeof(buf)) {
		w = sizeof(buf);
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
		SummarizeSize(ent->size, size);
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

static void SelectByName(struct directory_pane *p, const char *name)
{
	struct directory_entry *entry = VFS_EntryByName(p->dir, name);
	if (entry != NULL) {
		UI_ListPaneSelect(&p->pane, entry - p->dir->entries + 1);
	}
}

static void SelectBySerial(struct directory_pane *p, uint64_t serial_no)
{
	struct directory_entry *entry = VFS_EntryBySerial(p->dir, serial_no);
	if (entry != NULL) {
		UI_ListPaneSelect(&p->pane, entry - p->dir->entries + 1);
	}
}

void UI_DirectoryPaneSearch(void *p, const char *needle)
{
	const struct directory_entry *ent;
	struct directory_pane *dp = p;
	size_t haystack_len, needle_len = strlen(needle);
	int i, j;

	if (strlen(needle) == 0) {
		return;
	}

	if (!strcmp(needle, "..")) {
		dp->pane.selected = 0;
		dp->pane.window_offset = 0;
		return;
	}

	// Check for prefix first, so user can type entire lump name.
	for (i = 0; i < dp->dir->num_entries; i++) {
		ent = &dp->dir->entries[i];
		if (!strncasecmp(ent->name, needle, needle_len)) {
			dp->pane.selected = i + 1;
			dp->pane.window_offset = dp->pane.selected >= 10 ?
			    dp->pane.selected - 10 : 0;
			return;
		}
	}

	// Second time through, we look for a substring match.
	for (i = 0; i < dp->dir->num_entries; i++) {
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
				return;
			}
		}
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
		SelectBySerial(p, dirent->serial_no);
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

static void MoveLumps(struct directory_pane *p, struct wad_file *wf)
{
	struct wad_file_entry *dir;
	int i, numlumps;
	int insert_start, insert_end, insert_point;

	insert_start = UI_DirectoryPaneSelected(p) + 1;
	insert_end = insert_start + p->tagged.num_entries;
	insert_point = insert_start;

	W_AddEntries(wf, insert_start, p->tagged.num_entries);
	dir = W_GetDirectory(wf);
	numlumps = W_NumLumps(wf);

	for (i = 0; i < numlumps; i++) {
		if (i >= insert_start && i < insert_end) {
			continue;
		}
		if (!VFS_SetHas(&p->tagged, dir[i].serial_no)) {
			continue;
		}
		// This is a lump we want to move.
		memcpy(&dir[insert_point], &dir[i],
		       sizeof(struct wad_file_entry));
		++insert_point;

		W_DeleteEntry(wf, i);
		if (i <= insert_start) {
			--insert_start;
			--insert_end;
			--insert_point;
		}
		--numlumps;
		--i;
		dir = W_GetDirectory(wf);
	}

	UI_ListPaneSelect(&p->pane, insert_start + 1);
	VFS_Refresh(p->dir);
}

static void Keypress(void *directory_pane, int key)
{
	struct directory_pane *p = directory_pane;
	char *input_filename;
	struct file_set *tagged = UI_DirectoryPaneTagged(p);
	int selected = UI_DirectoryPaneSelected(p);

	if (key == KEY_F(3) || key == KEY_F(4)) {
		UI_MessageBox("Sorry, not implemented yet.");
		return;
	}
	if (p->dir->type == FILE_TYPE_WAD && key == KEY_F(2)) {
		if (p->tagged.num_entries == 0) {
			UI_MessageBox(
			    "You have not selected any lumps to move.");
			return;
		}
		MoveLumps(p, VFS_WadFile(p->dir));
		return;
	}
	if (key == KEY_F(6)) {
		char *old_name = p->dir->entries[selected].name;
		uint64_t serial_no = p->dir->entries[selected].serial_no;

		if (tagged->num_entries == 0) {
			UI_MessageBox(
			    "You have not selected anything to rename.");
			return;
		} else if (tagged->num_entries > 1) {
			UI_MessageBox(
			    "You can't rename more than one thing at once.");
			return;
		}

		input_filename = UI_TextInputDialogBox(
		    "Rename", 30, "New name for '%s'?", old_name);
		if (input_filename == NULL) {
			return;
		}
		VFS_Rename(p->dir, &p->dir->entries[selected],
		           input_filename);
		VFS_Refresh(p->dir);
		free(input_filename);
		SelectBySerial(p, serial_no);
		return;
	}
	if (p->dir->type == FILE_TYPE_WAD
	 && (key == KEY_F(7) || key == KEY_IC)) {
		struct wad_file *f = VFS_WadFile(p->dir);
		char *name = UI_TextInputDialogBox(
			"New lump", 8,
			"Enter name for new lump:");
		if (name == NULL) {
			return;
		}
		W_AddEntries(f, selected + 1, 1);
		W_SetLumpName(f, selected + 1, name);
		free(name);
		VFS_Refresh(p->dir);
		UI_ListPaneKeypress(p, KEY_DOWN);
		return;
	}
	if (p->dir->type == FILE_TYPE_DIR && key == KEY_F(7)) {
		char *filename;
		input_filename = UI_TextInputDialogBox(
		    "Make directory", 30, "Name for new directory?");
		if (input_filename == NULL) {
			return;
		}
		filename = StringJoin("/", p->dir->path, input_filename, NULL);
		mkdir(filename, 0777);
		VFS_Refresh(p->dir);
		SelectByName(p, input_filename);
		free(input_filename);
		free(filename);
		return;
	}
	// TODO: Delete all marked
	if (key == KEY_F(8) || key == KEY_DC) {
		char buf[64];
		int i;

		if (tagged->num_entries == 0) {
			UI_MessageBox(
			    "You have not selected anything to delete.");
			return;
		}

		VFS_DescribeSet(p->dir, tagged, buf, sizeof(buf));
		if (!UI_ConfirmDialogBox("Confirm Delete", "Delete %s?", buf)) {
			return;
		}
		// Note that there's a corner-case gotcha here. VFS serial
		// numbers for files are inode numbers, and through hardlinks
		// multiple files can have the same inode number. However,
		// the way things are implemented here, we only ever delete one
		// of each serial number. So the wrong file can end up being
		// deleted, but we'll never delete both.
		for (i = 0; i < tagged->num_entries; i++) {
			struct directory_entry *ent;
			ent = VFS_EntryBySerial(p->dir, tagged->entries[i]);
			if (ent == NULL) {
				continue;
			}
			VFS_Remove(p->dir, ent);
		}
		VFS_ClearSet(&p->tagged);
		VFS_Refresh(p->dir);
		if (UI_DirectoryPaneSelected(p) >= p->dir->num_entries) {
			UI_ListPaneSelect(&p->pane, p->dir->num_entries);
		}
		return;
	}
	if (key == KEY_F(10)) {
		VFS_ClearSet(&p->tagged);
		return;
	}
	if (key == ' ') {
		struct directory_entry *ent;
		if (p->pane.selected <= 0) {
			return;
		}
		ent = &p->dir->entries[UI_DirectoryPaneSelected(p)];
		if (VFS_SetHas(&p->tagged, ent->serial_no)) {
			VFS_RemoveFromSet(&p->tagged, ent->serial_no);
		} else {
			VFS_AddToSet(&p->tagged, ent->serial_no);
		}
		UI_ListPaneKeypress(p, KEY_DOWN);
		return;
	}

	UI_ListPaneKeypress(p, key);
}

struct directory_pane *UI_NewDirectoryPane(
	WINDOW *w, struct directory *dir)
{
	struct directory_pane *p;

	p = calloc(1, sizeof(struct directory_pane));
	UI_ListPaneInit(&p->pane, w, &directory_pane_funcs, p);
	p->pane.pane.keypress = Keypress;
	// TODO: Free
	UI_ListPaneSetTitle(&p->pane, PathBaseName(dir->path));
	p->dir = dir;
	// Select first item (assuming there is one):
	Keypress(&p->pane, KEY_DOWN);

	return p;
}
