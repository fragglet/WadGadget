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

#include "fs/vfile.h"

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

#define GENMIDI_FLAG_FIXED  0x0001 /* fixed pitch */
#define GENMIDI_FLAG_2VOICE 0x0004 /* double voice (OPL3) */

struct genmidi_instrument_header {
	uint16_t flags;
	uint8_t fine_tune;
	uint8_t fixed_note;
};

#define GENMIDI_MAX_INSTR_LEN 32

struct genmidi_instrument {
	char name[GENMIDI_MAX_INSTR_LEN];
	struct genmidi_instrument_header hdr;
	uint8_t voice1[NUM_INSTR_FIELDS];
	uint8_t voice2[NUM_INSTR_FIELDS];
};

struct genmidi_bank {
	struct genmidi_instrument instrs[NUM_GENMIDI_INSTRS];
	int modified_count;
};

struct action;
struct directory_entry;
struct file_type;

extern const struct file_type file_type_genmidi_bank;
extern const struct file_type file_type_genmidi_voice;
extern const struct action genmidi_export_action;
extern const struct action genmidi_clear_action;

bool GENMIDI_LoadBank(struct genmidi_bank *bank, VFILE *in);
bool GENMIDI_SaveBank(struct genmidi_bank *bank, VFILE *out);
void GENMIDI_ClearInstrument(struct genmidi_bank *bank,
                             struct genmidi_instrument *instr, bool voice2);

struct directory *GENMIDI_OpenDir(struct directory *parent,
                                  struct directory_entry *ent);
struct genmidi_bank *GENMIDI_DirGetBank(struct directory *dir);
bool GENMIDI_WriteSBI(struct genmidi_instrument *instr, bool voice2,
                      VFILE *out);

#endif /* #ifndef GENMIDI__GENMIDI_H_INCLUDED */
