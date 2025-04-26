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
#include <ncurses.h>

#include "common.h"
#include "fs/vfile.h"
#include "genmidi/genmidi.h"

#define HEADER_VALUE "SBI\x1a"

static const int sbi_field_mapping[] = {
    INSTR_M_AM_VIBRATO_EG,   // 0
    INSTR_C_AM_VIBRATO_EG,   // 1
    INSTR_M_KSL,             // 2 ; _VOLUME is |ed in below
    INSTR_C_KSL,             // 3 ; _VOLUME is |ed in below
    INSTR_M_ATTACK_DECAY,    // 4
    INSTR_C_ATTACK_DECAY,    // 5
    INSTR_M_SUSTAIN_RELEASE, // 6
    INSTR_C_SUSTAIN_RELEASE, // 7
    INSTR_M_WAVEFORM,        // 8
    INSTR_C_WAVEFORM,        // 9
    INSTR_FEEDBACK_FM,       // 10
};

bool GENMIDI_WriteSBI(struct genmidi_instrument *instr, bool voice2, VFILE *out)
{
	uint8_t *voicedata;
	uint8_t buf[16];
	int i;

	if (vfwrite(HEADER_VALUE, 4, 1, out) != 1 ||
	    vfwrite(instr->name, 32, 1, out) != 1) {
		return false;
	}

	if (voice2) {
		voicedata = instr->voice2;
	} else {
		voicedata = instr->voice1;
	}

	memset(buf, 0, sizeof(buf));
	for (i = 0; i < arrlen(sbi_field_mapping); ++i) {
		buf[i] = voicedata[sbi_field_mapping[i]];
	}

	buf[2] |= voicedata[INSTR_M_VOLUME];
	buf[3] |= voicedata[INSTR_C_VOLUME];
	buf[12] = voicedata[INSTR_NOTE_OFFSET];

	if ((instr->hdr.flags & GENMIDI_FLAG_FIXED) != 0) {
		buf[11] = 1;
		buf[13] = instr->hdr.fixed_note;
	}

	return vfwrite(buf, 16, 1, out) == 1;
}
