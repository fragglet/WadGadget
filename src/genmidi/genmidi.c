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

#include "common.h"
#include "fs/vfile.h"
#include "genmidi/genmidi.h"

#define HEADER_MAGIC "#OPL_II#"
#define HEADER_LEN   8

bool GENMIDI_LoadBank(struct genmidi_bank *bank, VFILE *in)
{
	uint8_t header_data[HEADER_LEN];
	int i;

	bank->modified_count = 0;

	if (vfread(header_data, HEADER_LEN, 1, in) != 1 ||
	    memcmp(header_data, HEADER_MAGIC, HEADER_LEN) != 0) {
		return false;
	}

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		struct genmidi_instrument *instr = &bank->instrs[i];
		if (vfread(&instr->hdr, sizeof(instr->hdr), 1, in) != 1 ||
		    vfread(&instr->voice1, NUM_INSTR_FIELDS, 1, in) != 1 ||
		    vfread(&instr->voice2, NUM_INSTR_FIELDS, 1, in) != 1) {
			return false;
		}
		SwapLE16(&instr->hdr.flags);
	}

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		struct genmidi_instrument *instr = &bank->instrs[i];
		if (vfread(&instr->name, GENMIDI_MAX_INSTR_LEN, 1, in) != 1) {
			return false;
		}
		// Ensure names are always NUL terminated
		instr->name[GENMIDI_MAX_INSTR_LEN - 1] = '\0';
	}

	return true;
}

bool GENMIDI_SaveBank(struct genmidi_bank *bank, VFILE *out)
{
	int i;

	if (vfwrite(HEADER_MAGIC, HEADER_LEN, 1, out) != 1) {
		return false;
	}

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		struct genmidi_instrument instr = bank->instrs[i];
		SwapLE16(&instr.hdr.flags);
		if (vfwrite(&instr.hdr, sizeof(instr.hdr), 1, out) != 1 ||
		    vfwrite(&instr.voice1, NUM_INSTR_FIELDS, 1, out) != 1 ||
		    vfwrite(&instr.voice2, NUM_INSTR_FIELDS, 1, out) != 1) {
			return false;
		}
	}

	for (i = 0; i < NUM_GENMIDI_INSTRS; ++i) {
		struct genmidi_instrument *instr = &bank->instrs[i];
		if (vfwrite(&instr->name, GENMIDI_MAX_INSTR_LEN, 1, out) != 1) {
			return false;
		}
	}

	return true;
}

void GENMIDI_ClearInstrument(struct genmidi_bank *bank, struct genmidi_instrument *instr, bool voice2)
{
	++bank->modified_count;

	// We delete the second voice by clearing the two-voice flag:
	if (voice2) {
		instr->hdr.flags &= ~GENMIDI_FLAG_2VOICE;
		return;
	}

	// We delete the first voice by overwriting it with the second:
	if ((instr->hdr.flags & GENMIDI_FLAG_2VOICE) != 0) {
		memcpy(instr->voice1, instr->voice2, NUM_INSTR_FIELDS);
		instr->hdr.flags &= ~GENMIDI_FLAG_2VOICE;
		return;
	}

	snprintf(instr->name, sizeof(instr->name), "(unused)");
	instr->hdr.flags = 0;
	memset(instr->voice1, 0, NUM_INSTR_FIELDS);
	instr->voice1[INSTR_C_VOLUME] = 0x3f;
	instr->voice1[INSTR_M_VOLUME] = 0x3f;
}
