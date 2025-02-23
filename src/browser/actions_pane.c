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

#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "ui/colors.h"
#include "ui/ui.h"

static const int key_ordering[] = {
    KEY_F(1),   KEY_F(2),   KEY_F(3),   KEY_F(4), KEY_F(5),
    KEY_F(6),   KEY_F(7),   KEY_F(8),   KEY_F(9), ' ',
    CTRL_('G'), KEY_F(10),  0,          '\t',     CTRL_('D'),
    CTRL_('P'), CTRL_('Z'), CTRL_('Y'), 27,       CTRL_('J'),
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

struct action_iter {
	struct actions_pane *p;
	int i, y, last_idx;
};

static void BeginActionIter(struct actions_pane *p, struct action_iter *it)
{
	it->p = p;
	it->i = 0;
	it->y = 1;
	it->last_idx = -1;
}

static const struct action *NextActionIter(struct action_iter *it, int *x,
                                           int *y, struct action_options *opts)
{
	const struct action *a, *result = NULL;

	while (it->i < arrlen(key_ordering)) {
		a = it->p->actions[it->i];
		if (a != NULL && (a->key != 0 || a->ctrl_key != 0)) {
			ExtractOptions(a->description, opts);
			if (opts->right_side && it->last_idx == it->i - 1) {
				*x = 15;
				*y = it->y - 1;
			} else {
				*x = 2;
				*y = it->y;
				++it->y;
			}
			result = a;
		}
		if (key_ordering[it->i] == 0) {
			++it->y;
		}
		it->last_idx = it->i;
		++it->i;
		if (result != NULL) {
			return result;
		}
	}

	return NULL;
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

static bool DrawActionsPane(void *pane)
{
	struct actions_pane *p = pane;
	const struct action *a;
	WINDOW *win = p->pane.window;
	struct action_options opts;
	struct action_iter it;
	int x, y;

	wbkgdset(win, COLOR_PAIR(PAIR_PANE_COLOR));
	werase(win);
	UI_DrawWindowBox(win);
	mvwaddstr(win, 0, 2, " Actions ");

	BeginActionIter(p, &it);
	while ((a = NextActionIter(&it, &x, &y, &opts)) != NULL) {
		DrawAction(p, x, y, a, &opts);
	}

	return true;
}

static doubleclick_continuation ActionsPaneMouseClick(void *_p, int x, int y)
{
	struct actions_pane *p = _p;
	struct action_iter it;
	const struct action *a, *matched = NULL;
	struct action_options opts;
	int ax, ay;

	BeginActionIter(p, &it);
	while ((a = NextActionIter(&it, &ax, &ay, &opts)) != NULL && ay <= y) {
		// If there are multiple actions on the line, we want to end
		// up with the last one on the line with x <= mouse x
		if (y == ay && x >= ax) {
			matched = a;
		}
	}

	if (matched != NULL) {
		matched->callback();
	}

	return NULL;
}

void B_ActionsPaneInit(struct actions_pane *pane, WINDOW *win)
{
	pane->pane.window = win;
	pane->pane.draw = DrawActionsPane;
	pane->pane.keypress = NULL;
	pane->pane.mouse_click = ActionsPaneMouseClick;
	pane->function_keys = true;
	memset(pane->actions, 0, sizeof(pane->actions));
}

void B_ActionsPaneSet(struct actions_pane *pane, const struct action **actions,
                      bool left_to_right, bool function_keys)
{
	const struct action *a;
	int i, j;

	pane->left_to_right = left_to_right;
	pane->function_keys = function_keys;
	memset(pane->actions, 0, sizeof(pane->actions));

	for (i = 0; actions[i] != NULL; i++) {
		a = actions[i];
		for (j = 0; j < arrlen(key_ordering); ++j) {
			if (key_ordering[j] != 0 &&
			    (key_ordering[j] == a->key ||
			     key_ordering[j] == CTRL_(a->ctrl_key))) {
				pane->actions[j] = a;
				break;
			}
		}
	}
}
