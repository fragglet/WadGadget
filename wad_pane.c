#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ui.h"
#include "list_pane.h"
#include "wad_pane.h"

struct wad_pane {
	struct list_pane pane;
	struct wad_file *f;
};

static const char *GetEntry(struct list_pane *l, unsigned int idx)
{
	static char buf[9];
	struct wad_pane *p = (struct wad_pane *) l;
	const struct wad_file_entry *directory = W_GetDirectory(p->f);
	snprintf(buf, sizeof(buf), "%-8s", directory[idx].name);
	return buf;
}

struct list_pane *UI_NewWadPane(WINDOW *pane, struct wad_file *f)
{
	struct wad_pane *p;
	p = calloc(1, sizeof(struct wad_pane));
	p->pane.pane = pane;
	p->pane.title = "foobar.wad";
	p->pane.get_entry_str = GetEntry;
	p->f = f;
	return &p->pane;
}

