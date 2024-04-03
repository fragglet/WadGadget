#include "pane.h"
#include "vfs.h"

struct action {
	char *key;
	char *description;
};

struct actions_pane {
	struct pane pane;
	int left_to_right;
	enum file_type active, other;
};

void UI_ActionsPaneInit(struct actions_pane *pane, WINDOW *win);
void UI_ActionsPaneSet(struct actions_pane *pane, enum file_type active,
                       enum file_type other, int left_to_right);
