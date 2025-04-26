//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <assert.h>
#include <curses.h>

#include "browser/browser.h"
#include "browser/directory_pane.h"
#include "common.h"
#include "fs/vfile.h"
#include "genmidi/genmidi.h"
#include "stringlib.h"
#include "ui/actions_bar.h"

static void ActionExportVoices(void)
{
	struct genmidi_bank *bank = GENMIDI_DirGetBank(active_pane->dir);
	struct genmidi_instrument *instr;
	struct directory_entry *ent;
	struct file_set *tagged = B_DirectoryPaneTagged(active_pane);
	int it = 0;
	VFILE *out;

	// TODO: Confirm overwrite?
	while ((ent = VFS_IterateSet(active_pane->dir, tagged, &it)) != NULL) {
		int index = ent - active_pane->dir->entries;
		char *filename;

		instr = &bank->instrs[index / 2];

		filename = StringJoin("", other_pane->dir->path, DIR_SEPARATOR_S,
		                      ent->name, ".sbi", NULL);
		out = vfwrapfile(fopen(filename, "wb"));
		GENMIDI_WriteSBI(instr, (index % 2) != 0, out);
		vfclose(out);
	}
	VFS_Refresh(other_pane->dir);
}

const struct action genmidi_export_action = {
    KEY_F(5), 'C', "Export", "> Export", ActionExportVoices,
};
