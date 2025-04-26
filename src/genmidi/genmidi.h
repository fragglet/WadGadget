//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef GENMIDI__GENMIDI_H_INCLUDED
#define GENMIDI__GENMIDI_H_INCLUDED

#include <stdbool.h>
#include <stdint.h>

#define NUM_GENMIDI_INSTRS 175 /* 128 + 47 percussion */

enum genmidi_instr_field {
	INSTR_M_AM_VIBRATO_EG,
	INSTR_M_ATTACK_DECAY,
	INSTR_M_SUSTAIN_RELEASE,
	INSTR_M_WAVEFORM,
	INSTR_M_KSL,
	INSTR_M_VOLUME,
	INSTR_FEEDBACK_FM,
	INSTR_C_AM_VIBRATO_EG,
	INSTR_C_ATTACK_DECAY,
	INSTR_C_SUSTAIN_RELEASE,
	INSTR_C_WAVEFORM,
	INSTR_C_KSL,
	INSTR_C_VOLUME,
	INSTR_NULL,
	INSTR_NOTE_OFFSET,
	INSTR_NOTE_OFFSET2,
	NUM_INSTR_FIELDS,
};

struct genmidi_instrument_header {
	uint16_t flags;
	uint8_t fine_tune;
	uint8_t fixed_note;
};

struct genmidi_instrument {
	char name[32];
	struct genmidi_instrument_header hdr;
	uint8_t voice1[NUM_INSTR_FIELDS];
	uint8_t voice2[NUM_INSTR_FIELDS];
};

struct genmidi_bank {
	struct genmidi_instrument instrs[NUM_GENMIDI_INSTRS];
};

struct directory_entry;

bool GENMIDI_LoadBank(struct genmidi_bank *bank, VFILE *in);
struct directory *GENMIDI_OpenDir(struct directory *parent,
                                  struct directory_entry *ent);

#endif /* #ifndef GENMIDI__GENMIDI_H_INCLUDED */
