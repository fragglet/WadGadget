//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "browser/actions_pane.h"

#include <string.h>
#include <stdbool.h>

#include "ui/colors.h"
#include "common.h"
#include "ui/ui.h"

static const int key_ordering[] = {
	KEY_F(1), KEY_F(2), KEY_F(3), KEY_F(4), KEY_F(5), KEY_F(6),
	KEY_F(7), KEY_F(8), KEY_F(9), ' ', CTRL_('G'), KEY_F(10),
	0,
	'\t', CTRL_('D'), CTRL_('P'),
	CTRL_('Z'), CTRL_('Y'),
	27, CTRL_('J'),
};

struct action_options {
	bool arrows;
	bool right_side;
	bool ellipsis;
	const char *desc;
};

static void ExtractOptions(const char *desc, struct action_options *opts)
{
	opts->arrows = false;
	opts->ellipsis = false;
	opts->right_side = false;

	for (;; ++desc) {
		if (*desc == '>') {
			opts->arrows = true;
		} else if (*desc == '|') {
			opts->right_side = true;
		} else if (*desc == '.') {
			opts->ellipsis = true;
		} else if (*desc != ' ') {
			break;
		}
	}

	opts->desc = desc;
}

static void DrawAction(struct actions_pane *p, int x, int y,
                       const struct action *action,
                       const struct action_options *opts)
{
	WINDOW *win = p->pane.window;

	wattron(win, A_BOLD);
	mvwaddstr(win, y, x, UI_ActionKeyDescription(action, p->function_keys));
	wattroff(win, A_BOLD);
	waddstr(win, " - ");

	if (opts->arrows && !p->left_to_right) {
		wattron(win, A_BOLD);
		waddstr(win, "<<< ");
		wattroff(win, A_BOLD);
	}
	waddstr(win, opts->desc);
	if (opts->ellipsis) {
		waddstr(win, "...");
	}
	if (opts->arrows && p->left_to_right) {
		wattron(win, A_BOLD);
		waddstr(win, " >>>");
		wattroff(win, A_BOLD);
	}
}

static int ShowAction(struct actions_pane *p, int y,
                      const struct action *action,
                      const struct action_options *opts, bool right_ok)
{
	int x, result;

	if (action->key == 0 && action->ctrl_key == 0) {
		return 0;
	}

	if (opts->right_side && right_ok) {
		x = 15;
		--y;
		result = 0;
	} else {
		x = 2;
		result = 1;
	}

	DrawAction(p, x, y, action, opts);

	return result;
}

static bool DrawActionsPane(void *pane)
{
	struct actions_pane *p = pane;
	const struct action *a;
	WINDOW *win = p->pane.window;
	struct action_options opts;
	int i, y, last_idx = -1;

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	UI_DrawWindowBox(win);
	mvwaddstr(win, 0, 2, " Actions ");

	for (i = 0, y = 1; i < arrlen(key_ordering); i++) {
		a = p->actions[i];
		if (a != NULL) {
			ExtractOptions(a->description, &opts);
			y += ShowAction(p, y, a, &opts, last_idx == i - 1);
		}
		if (key_ordering[i] == 0) {
			y++;
		}
		last_idx = i;
	}

	return true;
}

void B_ActionsPaneInit(struct actions_pane *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsPane;
	pane->pane.keypress = NULL;
	pane->pane.mouse_click = NULL;
	pane->function_keys = true;
	memset(pane->actions, 0, sizeof(pane->actions));
}

void B_ActionsPaneSet(struct actions_pane *pane,
                      const struct action **actions, bool left_to_right,
                      bool function_keys)
{
	const struct action *a;
	int i, j;

	pane->left_to_right = left_to_right;
	pane->function_keys = function_keys;
	memset(pane->actions, 0, sizeof(pane->actions));

	for (i = 0; actions[i] != NULL; i++) {
		a = actions[i];
		for (j = 0; j < arrlen(key_ordering); ++j) {
			if (key_ordering[j] != 0
			 && (key_ordering[j] == a->key
			  || key_ordering[j] == CTRL_(a->ctrl_key))) {
				pane->actions[j] = a;
				break;
			}
		}
	}
}
