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
		snprintf(buf, 10, " %lld", len);
	} else if (len < 1000000000) {
		snprintf(buf, 10, " %lldK", len / 1000);
	} else if (len < 1000000000000) {
		snprintf(buf, 10, " %lldM", len / 1000000);
	} else if (len < 1000000000000000) {
		snprintf(buf, 10, " %lldG", len / 1000000000);
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
		snprintf(buf, w, "^ %s",
		         "parent");
	//	         or_if_null(dp->blob_list->parent_dir, ""));
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
	int i;

	for (i = 0; i < p->dir->num_entries; i++) {
		if (!strcmp(p->dir->entries[i].name, name)) {
			UI_ListPaneSelect(&p->pane, i + 1);
			return;
		}
	}
}

static void SelectBySerial(struct directory_pane *p, uint64_t serial_no)
{
	int i;

	for (i = 0; i < p->dir->num_entries; i++) {
		if (p->dir->entries[i].serial_no == serial_no) {
			UI_ListPaneSelect(&p->pane, i + 1);
			return;
		}
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
		for (j = 0; j < haystack_len - needle_len; j++) {
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

void UI_DirectoryPaneTagged(struct directory_pane *p, struct file_set *set)
{
	if (p->tagged.num_entries > 0) {
		VFS_CopySet(set, &p->tagged);
	} else {
		int selected = UI_DirectoryPaneSelected(p);
		VFS_ClearSet(set);
		if (selected >= 0) {
			VFS_AddToSet(set, p->dir->entries[selected].serial_no);
		}
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

static void Keypress(void *directory_pane, int key)
{
	struct directory_pane *p = directory_pane;
	char *input_filename;
	int selected = UI_DirectoryPaneSelected(p);

	if (key == KEY_F(6) && selected >= 0) {
		char *old_name = p->dir->entries[selected].name;
		uint64_t serial_no = p->dir->entries[selected].serial_no;
		input_filename = UI_TextInputDialogBox(
		    "Rename", 30, "New name for '%s'?", old_name);
		if (input_filename == NULL) {
			return;
		}
		VFS_Rename(p->dir, &p->dir->entries[selected],
		           input_filename);
		free(input_filename);
		SelectBySerial(p, serial_no);
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
	if (key == KEY_F(8) && selected >= 0) {
		char *filename = p->dir->entries[selected].name;
		if (!UI_ConfirmDialogBox("Confirm Delete", "Delete '%s'?",
		                         filename)) {
			return;
		}
		VFS_Remove(p->dir, &p->dir->entries[selected]);
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
