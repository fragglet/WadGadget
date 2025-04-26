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

#include "fs/vfile.h"
#include "genmidi/genmidi.h"

#define HEADER_MAGIC "#OPL_II#"
#define HEADER_LEN   8

#define INSTR_NAME_LEN 32

bool GENMIDI_LoadBank(struct genmidi_bank *bank, VFILE *in)
{
	uint8_t header_data[HEADER_LEN];
	int i;

	if (vfread(header_data, HEADER_LEN, 1, in) != 1) {
		return false;
	}

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		struct genmidi_instrument *instr = &bank->instrs[i];
		if (vfread(&instr->hdr,
		           sizeof(struct genmidi_instrument_header), 1,
		           in) != 1 ||
		    vfread(&instr->voice1, NUM_INSTR_FIELDS, 1, in) != 1 ||
		    vfread(&instr->voice2, NUM_INSTR_FIELDS, 1, in) != 1) {
			return false;
		}
	}

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		struct genmidi_instrument *instr = &bank->instrs[i];
		if (vfread(&instr->name, INSTR_NAME_LEN, 1, in) != 1) {
			return false;
		}
		// Ensure names are always NUL terminated
		instr->name[INSTR_NAME_LEN - 1] = '\0';
	}

	return true;
}

int main(int argc, char *argv[])
{
	struct genmidi_bank bank;
	int i, j;
	VFILE *in = vfwrapfile(fopen("genmidi.op2", "rb"));

	assert(GENMIDI_LoadBank(&bank, in));
	vfclose(in);

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		printf("%3d: %s\n", i, bank.instrs[i].name);
		printf("\t");
		for (j = 0; j < NUM_INSTR_FIELDS; ++j) {
			printf("%02x ", bank.instrs[i].voice1[j]);
		}
		printf("\n\t");
		for (j = 0; j < NUM_INSTR_FIELDS; ++j) {
			printf("%02x ", bank.instrs[i].voice1[j]);
		}
		printf("\n");
	}
}
