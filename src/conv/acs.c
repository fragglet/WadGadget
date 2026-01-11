
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "conv/error.h"
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

static const struct acs_opcode *OpcodeByName(const char *name)
{
	int i;

	for (i = 0; i < arrlen(acs_opcodes); ++i) {
		if (!strcasecmp(name, acs_opcodes[i].name)) {
			return &acs_opcodes[i];
		}
	}

	return NULL;
}

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
		ConversionError("Lump too short (%d < 8)", l->data_len);
		return false;
	}
	memcpy(&offset, l->data + 4, sizeof(uint32_t));
	SwapLE32(&offset);

	// Decode the scripts table first:
	if (offset >= l->data_len - 8) {
		ConversionError("Invalid script table offset (%d >= %s)",
		                offset, l->data_len - 8);
		return false;
	}
	memcpy(&l->num_scripts, l->data + offset, sizeof(uint32_t));
	SwapLE32(&l->num_scripts);
	offset += 4;
	if (l->num_scripts >= l->data_len ||
	    offset + l->num_scripts * sizeof(struct script) > l->data_len) {
		ConversionError("Invalid number of scripts (%d; len=%d)",
		                l->num_scripts, l->data_len);
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
			ConversionError(
			    "Script %d: invalid offset (%d; len=%d)", i,
			    l->scripts[i].offset, l->data_len);
			return false;
		}
	}
	offset += sizeof(struct script) * l->num_scripts;

	// Decode the string offsets table.
	if (offset > l->data_len - 4) {
		ConversionError("Invalid string table offset (%d > %d)", offset,
		                l->data_len - 4);
		return false;
	}
	memcpy(&l->num_strings, l->data + offset, sizeof(uint32_t));
	SwapLE32(&l->num_strings);
	offset += 4;
	if (l->num_strings >= l->data_len ||
	    offset + l->num_strings * 4 > l->data_len) {
		ConversionError("Invalid number of strings (%d; len=%d)",
		                l->num_strings, l->data_len);
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
			ConversionError(
			    "String %d overruns lump end without NUL", i);
			return false;
		}
	}

	return true;
}

static bool MarkOpcodeSequence(struct behavior_lump *l, uint32_t offset)
{
	const struct acs_opcode *op;
	uint32_t opcode;
	uint32_t start_offset = offset;

	for (;;) {
		if (offset > l->data_len - 4) {
			ConversionError(
			    "Opcode sequence starting at 0x%x overruns "
			    "lump end (len=%d)",
			    start_offset, l->data_len);
			return false;
		}
		// Already processed this location?
		if ((l->metadata[offset] & LOCATION_OPCODE) != 0) {
			return true;
		}
		l->metadata[offset] |= LOCATION_OPCODE;

		// Decode the opcode:
		memcpy(&opcode, l->data + offset, sizeof(uint32_t));
		SwapLE32(&opcode);
		if (opcode >= arrlen(acs_opcodes)) {
			ConversionError("At address 0x%x, unknown opcode %d",
			                offset, opcode);
			return false;
		}
		op = &acs_opcodes[opcode];
		// If the last arg of this opcode is a location reference, we
		// must recurse to process the sequence at that location too:
		if ((op->flags & OPCODE_LOCATION_REF) != 0) {
			uint32_t jump_offset;
			memcpy(&jump_offset, l->data + offset + op->nargs * 4,
			       sizeof(uint32_t));
			SwapLE32(&jump_offset);
			l->metadata[jump_offset] |= LOCATION_JUMP_TARGET;
			if (!MarkOpcodeSequence(l, jump_offset)) {
				return false;
			}
		}
		// End of sequence?
		if ((op->flags & OPCODE_TERMINAL) != 0) {
			return true;
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
			ConversionError("Error while disassembling script %d",
			                i);
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

enum token_type {
	TOKEN_NAME,
	TOKEN_INT,
	TOKEN_COLON,
	TOKEN_STRING,
	TOKEN_EQUALS,
	TOKEN_OPEN_PAREN,
	TOKEN_CLOSE_PAREN,
	TOKEN_NEWLINE,
	TOKEN_EOF,
	TOKEN_ERROR,
};

struct token {
	enum token_type type;
	union {
		const char *s;
		int i;
	} x;
};

struct tokenizer {
	uint8_t *data;
	size_t data_len;
	char namebuf[32];
	unsigned int pos;
	unsigned int line_num;
};

static bool ReadEscapedChar(struct tokenizer *t, char *out)
{
	char hexbuf[3];
	char c;

	if (t->pos >= t->data_len) {
		return false;
	}

	c = t->data[t->pos];
	++t->pos;
	switch (c) {
	case 'n':
		*out = '\n';
		break;
	case '\\':
		*out = '\\';
		break;
	case '\"':
		*out = '\"';
		break;
	case 'x':
		if (t->pos + 2 > t->data_len) {
			return false;
		}
		hexbuf[0] = t->data[t->pos];
		hexbuf[1] = t->data[t->pos + 1];
		hexbuf[2] = '\0';
		t->pos += 2;
		*out = (char) strtol(hexbuf, NULL, 16);
		break;
	default:
		return false;
	}

	return true;
}

static struct token ReadStringToken(struct tokenizer *t)
{
	struct token result = {TOKEN_STRING};
	char *p = (char *) t->data + t->pos;

	// We store the unescaped string into the input buffer itself,
	// overwriting the original string.
	result.x.s = p;

	// Skip initial ":
	++t->pos;

	for (;;) {
		char c;
		if (t->pos >= t->data_len) {
			goto error;
		}

		c = t->data[t->pos];
		if (!isprint(c)) {
			goto error;
		}
		++t->pos;
		switch (c) {
		case '"':
			// End of string.
			*p = '\0';
			return result;
		case '\\':
			if (!ReadEscapedChar(t, p)) {
				goto error;
			}
			++p;
			break;
		default:
			*p = c;
			++p;
			break;
		}
	}

error:
	result.type = TOKEN_ERROR;
	return result;
}

static struct token ReadNumberToken(struct tokenizer *t)
{
	struct token result = {TOKEN_INT};
	result.x.i = 0;

	for (; t->pos < t->data_len; ++t->pos) {
		char c = t->data[t->pos];
		if (!isdigit(c)) {
			break;
		}
		result.x.i *= 10;
		result.x.i += c - '0';
	}

	return result;
}

static struct token ReadNameToken(struct tokenizer *t)
{
	struct token result = {TOKEN_NAME};
	int i;

	result.x.s = t->namebuf;

	for (i = 0; t->pos < t->data_len; ++i, ++t->pos) {
		char c = t->data[t->pos];
		if (!isalnum(c) && c != '_') {
			break;
		}
		if (i >= sizeof(t->namebuf) - 1) {
			result.type = TOKEN_ERROR;
			break;
		}
		t->namebuf[i] = c;
		t->namebuf[i + 1] = '\0';
	}

	return result;
}

static struct token NextToken(struct tokenizer *t)
{
	struct token result;
	char c;

	for (;;) {
		if (t->pos >= t->data_len) {
			result.type = TOKEN_EOF;
			return result;
		}

		c = t->data[t->pos];
		if (c == '\n' || !isspace(c)) {
			break;
		}
		++t->pos;
	}

	switch (c) {
	case ':':
		result.type = TOKEN_COLON;
		break;
	case '=':
		result.type = TOKEN_EQUALS;
		break;
	case '\n':
		result.type = TOKEN_NEWLINE;
		++t->line_num;
		break;
	case '(':
		result.type = TOKEN_OPEN_PAREN;
		break;
	case ')':
		result.type = TOKEN_CLOSE_PAREN;
		break;
	case '"':
		return ReadStringToken(t);
	default:
		if (isalpha(c)) {
			return ReadNameToken(t);
		} else if (isdigit(c)) {
			return ReadNumberToken(t);
		} else {
			result.type = TOKEN_ERROR;
		}
		break;
	}

	++t->pos;
	return result;
}

struct label {
	char *name;
	uint32_t location;
	uint32_t *fixups;
	size_t num_fixups;
};

struct assembler {
	struct tokenizer t;

	uint32_t *words;
	size_t num_words;

	struct script *scripts;
	size_t num_scripts;

	char **strings;
	size_t num_strings;

	struct label *labels;
	size_t num_labels;

	bool got_error;
};

static void AssembleError(struct assembler *a, const char *s, ...)
{
	char buf[80];
	va_list args;

	va_start(args, s);
	VStringPrintf(buf, sizeof(buf), s, args);
	va_end(args);

	ConversionError("Line %d: %s", a->t.line_num, buf);

	a->got_error = true;
}

static uint32_t *AppendWord(uint32_t **words, size_t *num_words)
{
	++*num_words;
	*words = checked_realloc(*words, *num_words * sizeof(uint32_t));
	return &(*words)[*num_words - 1];
}

static struct label *LabelByName(struct assembler *a, const char *name)
{
	struct label *l;
	int i;

	for (i = 0; i < a->num_labels; ++i) {
		if (!strcasecmp(a->labels[i].name, name)) {
			return &a->labels[i];
		}
	}

	// Create label on first reference:
	++a->num_labels;
	a->labels =
	    checked_realloc(a->labels, a->num_labels * sizeof(struct label));
	l = &a->labels[a->num_labels - 1];

	l->name = checked_strdup(name);
	l->location = 0;
	l->fixups = NULL;
	l->num_fixups = 0;
	return l;
}

static void InitAssembler(struct assembler *a, VFILE *in)
{
	memset(a, 0, sizeof(struct assembler));
	a->t.data = vfreadall(in, &a->t.data_len);
	a->t.pos = 0;
	a->t.line_num = 1;

	// Leave two words at the start of the lump for the lump header:
	AppendWord(&a->words, &a->num_words);
	AppendWord(&a->words, &a->num_words);
}

static void FreeAssembler(struct assembler *a)
{
	int i;

	free(a->t.data);
	free(a->words);
	free(a->scripts);

	for (i = 0; i < a->num_strings; ++i) {
		free(a->strings[i]);
	}
	free(a->strings);

	for (i = 0; i < a->num_labels; ++i) {
		free(a->labels[i].fixups);
	}
	free(a->labels);
}

static bool ExpectToken(struct assembler *a, enum token_type t, const char *s)
{
	struct token t2 = NextToken(&a->t);
	if (t2.type != t) {
		AssembleError(a, "Syntax error, expected %s", s);
		return false;
	}
	return true;
}

static bool AssembleLabel(struct assembler *a, struct token t)
{
	struct label *l;

	if (!ExpectToken(a, TOKEN_COLON, "colon")) {
		return false;
	}

	l = LabelByName(a, t.x.s);

	// We are defining a new label. This should be the first time we have
	// done this.
	if (l->location != 0) {
		AssembleError(a, "Label '%s' defined twice", t.x.s);
		return false;
	}

	l->location = a->num_words;
	return true;
}

static bool AssembleScriptStatement(struct assembler *a)
{
	struct token t = NextToken(&a->t);
	struct script *s;

	if (t.type != TOKEN_INT) {
		AssembleError(a, "Expected script number following 'Script'");
		return false;
	}

	++a->num_scripts;
	a->scripts =
	    checked_realloc(a->scripts, sizeof(struct script) * a->num_scripts);
	s = &a->scripts[a->num_scripts - 1];

	s->script_num = t.x.i;
	s->offset = a->num_words;

	t = NextToken(&a->t);
	switch (t.type) {
	case TOKEN_OPEN_PAREN:
		t = NextToken(&a->t);
		if (t.type != TOKEN_INT) {
			AssembleError(a, "Expected argument count");
			return false;
		}
		s->arg_count = t.x.i;
		if (s->arg_count > 3) {
			AssembleError(a, "Script may have 3 arguments max");
			return false;
		}
		return ExpectToken(a, TOKEN_CLOSE_PAREN, "')'") &&
		       ExpectToken(a, TOKEN_NEWLINE, "end of line");
	case TOKEN_NEWLINE:
		s->arg_count = 0;
		return true;
	default:
		AssembleError(a, "Expected end of line, or argument count "
		                 "in parentheses");
		return false;
	}
}

static bool AssembleStringStatement(struct assembler *a)
{
	struct token t = NextToken(&a->t);
	int string_id, new_num_strings;

	if (t.type != TOKEN_INT) {
		AssembleError(a, "Expected string number");
		return false;
	}

	string_id = t.x.i;
	new_num_strings = max(string_id + 1, a->num_strings);

	a->strings =
	    checked_realloc(a->strings, new_num_strings * sizeof(char *));
	while (a->num_strings < new_num_strings) {
		a->strings[a->num_strings] = NULL;
		++a->num_strings;
	}

	if (!ExpectToken(a, TOKEN_EQUALS, "'='")) {
		return false;
	}

	t = NextToken(&a->t);
	if (t.type != TOKEN_STRING) {
		AssembleError(a, "Expected string following '='");
		return false;
	}

	a->strings[string_id] = checked_strdup(t.x.s);
	return ExpectToken(a, TOKEN_NEWLINE, "end of line");
}

static bool AssembleInstruction(struct assembler *a)
{
	const struct acs_opcode *opcode;
	struct token t = NextToken(&a->t);
	struct label *l;
	uint32_t *w;
	unsigned int i;

	switch (t.type) {
	case TOKEN_EOF:
		return false;
	case TOKEN_NEWLINE:
		// Empty line
		return true;
	case TOKEN_NAME:
		break;
	default:
		AssembleError(a, "Expected instruction, 'Script' or 'String'");
		return false;
	}

	if (!strcasecmp(t.x.s, "Script")) {
		return AssembleScriptStatement(a);
	} else if (!strcasecmp(t.x.s, "String")) {
		return AssembleStringStatement(a);
	}

	opcode = OpcodeByName(t.x.s);

	// If we get a name and it is not the name of an opcode, the only
	// explanation is that it must be a label definition.
	if (opcode == NULL) {
		return AssembleLabel(a, t);
	}

	w = AppendWord(&a->words, &a->num_words);
	*w = opcode - acs_opcodes;

	for (i = 0; i < opcode->nargs; ++i) {
		t = NextToken(&a->t);
		switch (t.type) {
		case TOKEN_INT:
			w = AppendWord(&a->words, &a->num_words);
			*w = t.x.i;
			break;
		case TOKEN_NAME:
			l = LabelByName(a, t.x.s);
			w = AppendWord(&l->fixups, &l->num_fixups);
			*w = a->num_words;
			AppendWord(&a->words, &a->num_words);
			break;
		default:
			goto bad_args;
		}
	}

	if (!ExpectToken(a, TOKEN_NEWLINE, "end of line")) {
		goto bad_args;
	}

	return true;

bad_args:
	AssembleError(a, "Expecting %d arguments for %s instruction",
	              opcode->nargs, opcode->name);
	return false;
}

static bool ApplyFixups(struct assembler *a)
{
	struct label *l;
	unsigned int i, j;

	for (i = 0; i < a->num_labels; ++i) {
		l = &a->labels[i];
		if (l->location == 0) {
			AssembleError(a,
			              "Label '%s' referenced but not defined",
			              l->name);
			return false;
		}
		for (j = 0; j < l->num_fixups; ++j) {
			a->words[l->fixups[j]] = l->location * 4;
		}
	}

	return true;
}

static void WriteWords(VFILE *out, uint32_t *words, size_t num_words)
{
	unsigned int i;
	uint32_t val;

	for (i = 0; i < num_words; ++i) {
		val = words[i];
		SwapLE32(&val);
		vfwrite(&val, sizeof(uint32_t), 1, out);
	}
}

static void WriteWord(VFILE *out, uint32_t w)
{
	WriteWords(out, &w, 1);
}

static uint32_t *WriteStrings(struct assembler *a, VFILE *out)
{
	uint32_t *result = checked_calloc(a->num_strings, sizeof(uint32_t));
	unsigned int i;

	for (i = 0; i < a->num_strings; ++i) {
		result[i] = (uint32_t) vftell(out);
		vfwrite(a->strings[i], 1, strlen(a->strings[i]) + 1, out);
	}

	return result;
}

static void WriteScriptsTable(struct assembler *a, VFILE *out)
{
	struct script *s;
	uint32_t vals[3];
	unsigned int i;

	WriteWord(out, a->num_scripts);

	for (i = 0; i < a->num_scripts; ++i) {
		s = &a->scripts[i];
		vals[0] = s->script_num;
		vals[1] = s->offset * 4;
		vals[2] = s->arg_count;
		WriteWords(out, vals, 3);
	}
}

static VFILE *WriteAssembledLump(struct assembler *a)
{
	VFILE *result = vfopenmem(NULL, 0);
	uint32_t *string_locs;
	uint32_t script_dir_offset;
	uint32_t val;

	WriteWords(result, a->words, a->num_words);
	string_locs = WriteStrings(a, result);

	script_dir_offset = (uint32_t) vftell(result);
	WriteScriptsTable(a, result);

	val = a->num_strings;
	WriteWord(result, val);
	WriteWords(result, string_locs, a->num_strings);

	// Go back to the start, and write the header.
	vfseek(result, 0, SEEK_SET);
	vfwrite("ACS", 1, 4, result);
	WriteWord(result, script_dir_offset);

	// Rewind once again and we're done.
	vfseek(result, 0, SEEK_SET);
	return result;
}

VFILE *ACS_Assemble(VFILE *in)
{
	VFILE *out;
	struct assembler a;

	InitAssembler(&a, in);
	vfclose(in);

	while (AssembleInstruction(&a)) {
	}

	if (a.got_error || !ApplyFixups(&a)) {
		FreeAssembler(&a);
		return NULL;
	}

	out = WriteAssembledLump(&a);
	FreeAssembler(&a);

	return out;
}
