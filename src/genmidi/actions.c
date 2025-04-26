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
#include <ncurses.h>

#include "browser/browser.h"
#include "browser/directory_pane.h"
#include "common.h"
#include "fs/vfile.h"
#include "genmidi/genmidi.h"
#include "ui/actions_bar.h"

static void ActionExportVoice(void)
{
	struct genmidi_bank *bank = GENMIDI_DirGetBank(active_pane->dir);
	struct genmidi_instrument *instr;
	int selected = B_DirectoryPaneSelected(active_pane);
	VFILE *out;

	if (selected < 0) {
		return;
	}

	instr = &bank->instrs[selected / 2];

	// TODO: Write to the correct directory
	out = vfwrapfile(fopen("foo.sbi", "wb"));
	GENMIDI_WriteSBI(instr, (selected % 2) != 0, out);
	vfclose(out);
}

const struct action genmidi_export_action = {
    KEY_F(5), 'C', "Export", "> Export", ActionExportVoice,
};
