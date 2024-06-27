//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

struct plaintext_pager_config {
	struct pager_config pc;
	char **lines;
};

bool P_InitPlaintextConfig(const char *title,
                           struct plaintext_pager_config *cfg, VFILE *input);
void P_FreePlaintextConfig(struct plaintext_pager_config *cfg);
bool P_RunPlaintextPager(const char *title, VFILE *input);
