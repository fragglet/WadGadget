//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef PAGER__PLAINTEXT_H_INCLUDED
#define PAGER__PLAINTEXT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fs/vfile.h"
#include "pager/pager.h"

struct plaintext_pager_config {
	struct pager_config pc;
	char **lines;
	uint8_t *data;
	size_t data_len;
	void *hexdump_config;
	bool want_edit;
};

enum plaintext_pager_result {
	PLAINTEXT_PAGER_FAILURE,
	PLAINTEXT_PAGER_DONE,
	PLAINTEXT_PAGER_WANT_EDIT,
};

bool P_InitPlaintextConfig(const char *title, bool editable,
                           struct plaintext_pager_config *cfg, VFILE *input);
void P_FreePlaintextConfig(struct plaintext_pager_config *cfg);
char **P_PlaintextLines(const char *data, size_t data_len, size_t *num_lines);
enum plaintext_pager_result P_RunPlaintextPager(
	const char *title, VFILE *input, bool editable);

#endif /* #ifndef PAGER__PLAINTEXT_H_INCLUDED */
