#include "pane.h"

enum action_pane_type { ACTION_PANE_DIR, ACTION_PANE_WAD };

struct action {
	char *key;
	char *description;
};

struct actions_pane {
	struct pane pane;
	int left_to_right;
	enum action_pane_type active, other;
};

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void UI_ActionsPaneSet(struct actions_pane *pane, enum action_pane_type active,
                       enum action_pane_type other, int left_to_right);
