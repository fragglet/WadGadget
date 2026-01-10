
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs/vfile.h"
#include "stringlib.h"

// If set, there is an opcode at this location:
#define LOCATION_OPCODE 0x01

// If set, this location is the target of a goto-type instruction:
#define LOCATION_JUMP_TARGET 0x02

// If set, at least one script starts at this location:
#define LOCATION_SCRIPT_START 0x04

// If set in opcode.flags, the last argument to this opcode is a reference to
// a code location that the instruction can jump to (ie. it's a goto-type
// instruction):
#define OPCODE_LOCATION_REF 0x01

// If set in opcode.flags, this is a "terminal" instruction and we should not
// decode any subsequent ones.
#define OPCODE_TERMINAL 0x02

struct script {
	uint32_t script_num;
	uint32_t offset;
	uint32_t arg_count;
};

struct behavior_lump {
	uint8_t *data;
	size_t data_len;

	struct script *scripts;
	uint32_t num_scripts;
	uint32_t *string_offsets;
	uint32_t num_strings;

	// One byte per byte of the input lump, with bits set to indicate if
	// that location is the start of an instruction, plus other.
	uint8_t *metadata;
};

struct acs_opcode {
	const char *name;
	int nargs;
	int flags;
};

static const struct acs_opcode acs_opcodes[] = {
    {"NOP",                 0, 0                                    },
    {"Terminate",           0, OPCODE_TERMINAL                      },
    {"Suspend",             0, 0                                    },
    {"PushNumber",          1, 0                                    },
    {"LSpec1",              1, 0                                    },
    {"LSpec2",              1, 0                                    },
    {"LSpec3",              1, 0                                    },
    {"LSpec4",              1, 0                                    },
    {"LSpec5",              1, 0                                    },
    {"LSpec1Direct",        2, 0                                    },
    {"LSpec2Direct",        3, 0                                    },
    {"LSpec3Direct",        4, 0                                    },
    {"LSpec4Direct",        5, 0                                    },
    {"LSpec5Direct",        6, 0                                    },
    {"Add",                 0, 0                                    },
    {"Subtract",            0, 0                                    },
    {"Multiply",            0, 0                                    },
    {"Divide",              0, 0                                    },
    {"Modulus",             0, 0                                    },
    {"EQ",                  0, 0                                    },
    {"NE",                  0, 0                                    },
    {"LT",                  0, 0                                    },
    {"GT",                  0, 0                                    },
    {"LE",                  0, 0                                    },
    {"GE",                  0, 0                                    },
    {"AssignScriptVar",     1, 0                                    },
    {"AssignMapVar",        1, 0                                    },
    {"AssignWorldVar",      1, 0                                    },
    {"PushScriptVar",       1, 0                                    },
    {"PushMapVar",          1, 0                                    },
    {"PushWorldVar",        1, 0                                    },
    {"AddScriptVar",        1, 0                                    },
    {"AddMapVar",           1, 0                                    },
    {"AddWorldVar",         1, 0                                    },
    {"SubScriptVar",        1, 0                                    },
    {"SubMapVar",           1, 0                                    },
    {"SubWorldVar",         1, 0                                    },
    {"MulScriptVar",        1, 0                                    },
    {"MulMapVar",           1, 0                                    },
    {"MulWorldVar",         1, 0                                    },
    {"DivScriptVar",        1, 0                                    },
    {"DivMapVar",           1, 0                                    },
    {"DivWorldVar",         1, 0                                    },
    {"ModScriptVar",        1, 0                                    },
    {"ModMapVar",           1, 0                                    },
    {"ModWorldVar",         1, 0                                    },
    {"IncScriptVar",        1, 0                                    },
    {"IncMapVar",           1, 0                                    },
    {"IncWorldVar",         1, 0                                    },
    {"DecScriptVar",        1, 0                                    },
    {"DecMapVar",           1, 0                                    },
    {"DecWorldVar",         1, 0                                    },
    {"Goto",                1, OPCODE_LOCATION_REF | OPCODE_TERMINAL},
    {"IfGoto",              1, OPCODE_LOCATION_REF                  },
    {"Drop",                0, 0                                    },
    {"Delay",               0, 0                                    },
    {"DelayDirect",         1, 0                                    },
    {"Random",              0, 0                                    },
    {"RandomDirect",        2, 0                                    },
    {"ThingCount",          0, 0                                    },
    {"ThingCountDirect",    2, 0                                    },
    {"TagWait",             0, 0                                    },
    {"TagWaitDirect",       1, 0                                    },
    {"PolyWait",            0, 0                                    },
    {"PolyWaitDirect",      1, 0                                    },
    {"ChangeFloor",         0, 0                                    },
    {"ChangeFloorDirect",   2, 0                                    },
    {"ChangeCeiling",       0, 0                                    },
    {"ChangeCeilingDirect", 2, 0                                    },
    {"Restart",             0, OPCODE_TERMINAL                      },
    {"AndLogical",          0, 0                                    },
    {"OrLogical",           0, 0                                    },
    {"AndBitwise",          0, 0                                    },
    {"OrBitwise",           0, 0                                    },
    {"EorBitwise",          0, 0                                    },
    {"NegateLogical",       0, 0                                    },
    {"LShift",              0, 0                                    },
    {"RShift",              0, 0                                    },
    {"UnaryMinus",          0, 0                                    },
    {"IfNotGoto",           1, OPCODE_LOCATION_REF                  },
    {"LineSide",            0, 0                                    },
    {"ScriptWait",          0, 0                                    },
    {"ScriptWaitDirect",    1, 0                                    },
    {"ClearLineSpecial",    0, 0                                    },
    {"CaseGoto",            2, OPCODE_LOCATION_REF                  },
    {"BeginPrint",          0, 0                                    },
    {"EndPrint",            0, 0                                    },
    {"PrintString",         0, 0                                    },
    {"PrintNumber",         0, 0                                    },
    {"PrintCharacter",      0, 0                                    },
    {"PlayerCount",         0, 0                                    },
    {"GameType",            0, 0                                    },
    {"GameSkill",           0, 0                                    },
    {"Timer",               0, 0                                    },
    {"SectorSound",         0, 0                                    },
    {"AmbientSound",        0, 0                                    },
    {"SoundSequence",       0, 0                                    },
    {"SetLineTexture",      0, 0                                    },
    {"SetLineBlocking",     0, 0                                    },
    {"SetLineSpecial",      0, 0                                    },
    {"ThingSound",          0, 0                                    },
    {"EndPrintBold",        0, 0                                    },
};

static void FreeBehaviorLump(struct behavior_lump *l)
{
	free(l->scripts);
	free(l->string_offsets);
	free(l->metadata);
}

static bool DecodeTables(struct behavior_lump *l)
{
	uint32_t offset;
	unsigned int i, j;

	l->scripts = NULL;
	l->string_offsets = NULL;

	if (l->data_len < 8) {
		return false;
	}
	memcpy(&offset, l->data + 4, sizeof(uint32_t));
	SwapLE32(&offset);

	// Decode the scripts table first:
	if (offset >= l->data_len - 8) {
		return false;
	}
	memcpy(&l->num_scripts, l->data + offset, sizeof(uint32_t));
	SwapLE32(&l->num_scripts);
	offset += 4;
	if (l->num_scripts >= l->data_len ||
	    offset + l->num_scripts * sizeof(struct script) > l->data_len) {
		return false;
	}
	l->scripts = checked_calloc(l->num_scripts, sizeof(struct script));
	memcpy(l->scripts, l->data + offset,
	       sizeof(struct script) * l->num_scripts);
	for (i = 0; i < l->num_scripts; ++i) {
		SwapLE32(&l->scripts[i].script_num);
		SwapLE32(&l->scripts[i].offset);
		SwapLE32(&l->scripts[i].arg_count);
		if (l->scripts[i].offset > l->data_len - 4) {
			return false;
		}
	}
	offset += sizeof(struct script) * l->num_scripts;

	// Decode the string offsets table.
	if (offset > l->data_len - 4) {
		return false;
	}
	memcpy(&l->num_strings, l->data + offset, sizeof(uint32_t));
	SwapLE32(&l->num_strings);
	offset += 4;
	if (l->num_strings >= l->data_len ||
	    offset + l->num_strings * 4 > l->data_len) {
		return false;
	}
	l->string_offsets = checked_calloc(l->num_strings, sizeof(uint32_t));
	memcpy(l->string_offsets, l->data + offset,
	       l->num_strings * sizeof(uint32_t));
	for (i = 0; i < l->num_strings; ++i) {
		SwapLE32(&l->string_offsets[i]);
		// Check the string really is NUL-terminated:
		for (j = l->string_offsets[i]; j < l->data_len; ++j) {
			if (l->data[j] == '\0') {
				break;
			}
		}
		if (j >= l->data_len) {
			return false;
		}
	}

	return true;
}

static bool MarkOpcodeSequence(struct behavior_lump *l, uint32_t offset)
{
	const struct acs_opcode *op;
	uint32_t opcode;

	for (;;) {
		if (offset > l->data_len - 4) {
			return false;
		}
		// Already processed this location?
		if (l->metadata[offset] != 0) {
			return true;
		}
		l->metadata[offset] |= LOCATION_OPCODE;

		// Decode the opcode:
		memcpy(&opcode, l->data + offset, sizeof(uint32_t));
		SwapLE32(&opcode);
		if (opcode >= arrlen(acs_opcodes)) {
			return false;
		}
		op = &acs_opcodes[opcode];
		// End of sequence?
		if ((op->flags & OPCODE_TERMINAL) != 0) {
			return true;
		}
		// If the last arg of this opcode is a location reference, we
		// must recurse to process the sequence at that location too:
		if ((op->flags & OPCODE_LOCATION_REF) != 0) {
			uint32_t jump_offset;
			memcpy(&jump_offset, l->data + offset + op->nargs * 4,
			       sizeof(uint32_t));
			SwapLE32(&jump_offset);
			if (!MarkOpcodeSequence(l, jump_offset)) {
				return false;
			}
			l->metadata[offset] |= LOCATION_JUMP_TARGET;
		}
		offset += 4 * (op->nargs + 1);
	}
	return true;
}

static bool MarkLocations(struct behavior_lump *l)
{
	unsigned int i;

	l->metadata = checked_calloc(l->data_len, 1);

	for (i = 0; i < l->num_scripts; ++i) {
		if (!MarkOpcodeSequence(l, l->scripts[i].offset)) {
			return false;
		}
		l->metadata[l->scripts[i].offset] |= LOCATION_SCRIPT_START;
	}

	return true;
}

static bool DecodeLump(struct behavior_lump *l, uint8_t *data, size_t data_len)
{
	memset(l, 0, sizeof(*l));
	l->data = data;
	l->data_len = data_len;
	if (!DecodeTables(l) || !MarkLocations(l)) {
		FreeBehaviorLump(l);
		return false;
	}
	return true;
}

static void Printf(VFILE *out, const char *s, ...)
{
	char buf[32];
	va_list args;

	va_start(args, s);
	VStringPrintf(buf, sizeof(buf), s, args);
	va_end(args);

	vfwrite(buf, 1, strlen(buf), out);
}

static void DumpScriptsForAddress(VFILE *out, struct behavior_lump *l,
                                  uint32_t addr)
{
	const struct script *s;
	unsigned int i;

	Printf(out, "\n");
	for (i = 0; i < l->num_scripts; ++i) {
		s = &l->scripts[i];
		if (s->offset == addr) {
			Printf(out, "Script %d", s->script_num);
			if (s->arg_count > 0) {
				Printf(out, " (%d)", s->arg_count);
			}
			Printf(out, "\n");
		}
	}
}

static void DumpInstruction(VFILE *out, uint8_t *data)
{
	uint32_t opcode, val;
	const struct acs_opcode *op;
	int i;

	memcpy(&opcode, data, sizeof(uint32_t));
	SwapLE32(&opcode);
	data += 4;
	op = &acs_opcodes[opcode];

	Printf(out, "        %s", op->name);

	if (op->nargs > 0) {
		for (i = strlen(op->name); i < 15; ++i) {
			Printf(out, " ");
		}
	}

	for (i = 0; i < op->nargs; ++i) {
		memcpy(&val, data, sizeof(uint32_t));
		SwapLE32(&val);
		data += 4;

		if (i == op->nargs - 1 &&
		    (op->flags & OPCODE_LOCATION_REF) != 0) {
			Printf(out, " loc_%x", val);
		} else {
			Printf(out, " %d", val);
		}
	}
	Printf(out, "\n");
}

static void DumpString(VFILE *out, char *s)
{
	Printf(out, "\"");
	for (; *s != '\0'; ++s) {
		switch (*s) {
		case '\n':
			Printf(out, "\\n");
			break;
		case '\\':
		case '\"':
			Printf(out, "\\%c", *s);
			break;
		default:
			if (*s < 0x20 || *s >= 0x80) {
				Printf(out, "\\x%02x", *s);
			} else {
				Printf(out, "%c", *s);
			}
		}
	}
	Printf(out, "\"");
}

static void Dump(VFILE *out, struct behavior_lump *l)
{
	int i;

	for (i = 0; i < l->num_strings; ++i) {
		Printf(out, "String %d = ", i);
		DumpString(out, (char *) (l->data + l->string_offsets[i]));
		Printf(out, "\n");
	}
	for (i = 0; i < l->data_len; ++i) {
		if ((l->metadata[i] & LOCATION_SCRIPT_START) != 0) {
			DumpScriptsForAddress(out, l, i);
		}
		if ((l->metadata[i] & LOCATION_JUMP_TARGET) != 0) {
			Printf(out, "    loc_%x:\n", i);
		}
		if ((l->metadata[i] & LOCATION_OPCODE) != 0) {
			DumpInstruction(out, l->data + i);
		}
	}
}

VFILE *ACS_Disassemble(VFILE *in)
{
	struct behavior_lump l;
	VFILE *result;
	uint8_t *data;
	size_t data_len;

	data = vfreadall(in, &data_len);
	vfclose(in);

	if (!DecodeLump(&l, data, data_len)) {
		free(data);
		return NULL;
	}

	result = vfopenmem(NULL, 0);
	Dump(result, &l);

	free(data);
	FreeBehaviorLump(&l);

	vfseek(result, 0, SEEK_SET);
	return result;
}
