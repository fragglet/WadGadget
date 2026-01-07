
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

// If set in opcode.flags, the last argument to this opcode is a reference to
// a code location that the instruction can jump to (ie. it's a goto-type
// instruction):
#define OPCODE_LOCATION_REF  0x01

// If set in opcode.flags, this is a "terminal" instruction and we should not
// decode any subsequent ones.
#define OPCODE_TERMINAL      0x02

struct script {
	uint32_t script_num;
	uint32_t offset;
	uint32_t arg_count;
};

struct behavior_lump {
	struct script *scripts;
	uint32_t num_scripts;
	uint32_t *string_offsets;
	uint32_t num_strings;
};

struct acs_opcode {
	const char *name;
	int nargs;
	int flags;
};

const struct acs_opcode acs_opcodes[] = {
	{"NOP",                  0, 0},
	{"Terminate",            0, OPCODE_TERMINAL},
	{"Suspend",              0, 0},
	{"PushNumber",           1, 0},
	{"LSpec1",               1, 0},
	{"LSpec2",               1, 0},
	{"LSpec3",               1, 0},
	{"LSpec4",               1, 0},
	{"LSpec5",               1, 0},
	{"LSpec1Direct",         2, 0},
	{"LSpec2Direct",         3, 0},
	{"LSpec3Direct",         4, 0},
	{"LSpec4Direct",         5, 0},
	{"LSpec5Direct",         6, 0},
	{"Add",                  0, 0},
	{"Subtract",             0, 0},
	{"Multiply",             0, 0},
	{"Divide",               0, 0},
	{"Modulus",              0, 0},
	{"EQ",                   0, 0},
	{"NE",                   0, 0},
	{"LT",                   0, 0},
	{"GT",                   0, 0},
	{"LE",                   0, 0},
	{"GE",                   0, 0},
	{"AssignScriptVar",      1, 0},
	{"AssignMapVar",         1, 0},
	{"AssignWorldVar",       1, 0},
	{"PushScriptVar",        1, 0},
	{"PushMapVar",           1, 0},
	{"PushWorldVar",         1, 0},
	{"AddScriptVar",         1, 0},
	{"AddMapVar",            1, 0},
	{"AddWorldVar",          1, 0},
	{"SubScriptVar",         1, 0},
	{"SubMapVar",            1, 0},
	{"SubWorldVar",          1, 0},
	{"MulScriptVar",         1, 0},
	{"MulMapVar",            1, 0},
	{"MulWorldVar",          1, 0},
	{"DivScriptVar",         1, 0},
	{"DivMapVar",            1, 0},
	{"DivWorldVar",          1, 0},
	{"ModScriptVar",         1, 0},
	{"ModMapVar",            1, 0},
	{"ModWorldVar",          1, 0},
	{"IncScriptVar",         1, 0},
	{"IncMapVar",            1, 0},
	{"IncWorldVar",          1, 0},
	{"DecScriptVar",         1, 0},
	{"DecMapVar",            1, 0},
	{"DecWorldVar",          1, 0},
	{"Goto",                 1, OPCODE_LOCATION_REF|OPCODE_TERMINAL},
	{"IfGoto",               1, OPCODE_LOCATION_REF},
	{"Drop",                 0, 0},
	{"Delay",                0, 0},
	{"DelayDirect",          1, 0},
	{"Random",               0, 0},
	{"RandomDirect",         2, 0},
	{"ThingCount",           0, 0},
	{"ThingCountDirect",     2, 0},
	{"TagWait",              0, 0},
	{"TagWaitDirect",        1, 0},
	{"PolyWait",             0, 0},
	{"PolyWaitDirect",       1, 0},
	{"ChangeFloor",          0, 0},
	{"ChangeFloorDirect",    2, 0},
	{"ChangeCeiling",        0, 0},
	{"ChangeCeilingDirect",  2, 0},
	{"Restart",              0, OPCODE_TERMINAL},
	{"AndLogical",           0, 0},
	{"OrLogical",            0, 0},
	{"AndBitwise",           0, 0},
	{"OrBitwise",            0, 0},
	{"EorBitwise",           0, 0},
	{"NegateLogical",        0, 0},
	{"LShift",               0, 0},
	{"RShift",               0, 0},
	{"UnaryMinus",           0, 0},
	{"IfNotGoto",            1, OPCODE_LOCATION_REF},
	{"LineSide",             0, 0},
	{"ScriptWait",           0, 0},
	{"ScriptWaitDirect",     1, 0},
	{"ClearLineSpecial",     0, 0},
	{"CaseGoto",             2, OPCODE_LOCATION_REF},
	{"BeginPrint",           0, 0},
	{"EndPrint",             0, 0},
	{"PrintString",          0, 0},
	{"PrintNumber",          0, 0},
	{"PrintCharacter",       0, 0},
	{"PlayerCount",          0, 0},
	{"GameType",             0, 0},
	{"GameSkill",            0, 0},
	{"Timer",                0, 0},
	{"SectorSound",          0, 0},
	{"AmbientSound",         0, 0},
	{"SoundSequence",        0, 0},
	{"SetLineTexture",       0, 0},
	{"SetLineBlocking",      0, 0},
	{"SetLineSpecial",       0, 0},
	{"ThingSound",           0, 0},
	{"EndPrintBold",         0, 0},
};

static bool DecodeTables(struct behavior_lump *l, uint8_t *data,
                         size_t data_len)
{
	uint32_t offset;
	unsigned int i, j;

	l->scripts = NULL;
	l->string_offsets = NULL;

	if (data_len < 8) {
		goto fail;
	}
	memcpy(&offset, data + 4, sizeof(uint32_t));
	SwapLE32(&offset);

	// Decode the scripts table first:
	if (offset >= data_len - 8) {
		goto fail;
	}
	memcpy(&l->num_scripts, data + offset, sizeof(uint32_t));
	SwapLE32(&l->num_scripts);
	offset += 4;
	if (l->num_scripts >= data_len
	 || offset + l->num_scripts * sizeof(struct script) > data_len) {
		goto fail;
	}
	l->scripts = checked_calloc(l->num_scripts, sizeof(struct script));
	memcpy(l->scripts, data + offset,
	       sizeof(struct script) * l->num_scripts);
	for (i = 0; i < l->num_scripts; ++i) {
		SwapLE32(&l->scripts[i].script_num);
		SwapLE32(&l->scripts[i].offset);
		SwapLE32(&l->scripts[i].arg_count);
		if (l->scripts[i].offset > data_len - 4) {
			goto fail;
		}
	}
	offset += sizeof(struct script) * l->num_scripts;

	// Decode the string offsets table.
	if (offset > data_len - 4) {
		goto fail;
	}
	memcpy(&l->num_strings, data + offset, sizeof(uint32_t));
	SwapLE32(&l->num_strings);
	offset += 4;
	if (l->num_strings >= data_len
	 || offset + l->num_strings * 4 > data_len) {
		goto fail;
	}
	l->string_offsets = checked_calloc(l->num_strings, sizeof(uint32_t));
	memcpy(l->string_offsets, data + offset,
	       l->num_strings * sizeof(uint32_t));
	for (i = 0; i < l->num_strings; ++i) {
		SwapLE32(&l->string_offsets[i]);
		// Check the string really is NUL-terminated:
		for (j = l->string_offsets[i]; j < data_len; ++j) {
			if (data[j] == '\0') {
				break;
			}
		}
		if (j >= data_len) {
			goto fail;
		}
	}
	return true;

fail:
	free(l->scripts);
	free(l->string_offsets);
	return false;
}
