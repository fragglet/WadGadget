//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "struct.h"

#define arrlen(x) (sizeof(x) / sizeof(*(x)))

// TODO: Byte swap
static uint8_t read_uint8(uint8_t *data) { return *data; }
static uint16_t read_uint16(uint16_t *data) { return *data; }
static uint32_t read_uint32(uint32_t *data) { return *data; }
static int8_t read_int8(int8_t *data) { return *data; }
static int16_t read_int16(int16_t *data) { return *data; }
static int32_t read_int32(int32_t *data) { return *data; }

static size_t field_size_1(const struct struct_field *field) { return 1; }
static size_t field_size_2(const struct struct_field *field) { return 2; }
static size_t field_size_4(const struct struct_field *field) { return 4; }

static void decode_int8(const struct struct_field *field, void *data,
                        char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%d", read_int8(data));
}

static void decode_int16(const struct struct_field *field, void *data,
                         char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%d", read_int16(data));
}

static void decode_int32(const struct struct_field *field, void *data,
                         char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%d", read_int32(data));
}

static void decode_uint8(const struct struct_field *field, void *data,
                         char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%d", read_uint8(data));
}

static void decode_uint16(const struct struct_field *field, void *data,
                          char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%d", read_uint16(data));
}

static void decode_uint32(const struct struct_field *field, void *data,
                          char *buf, size_t buf_len)
{
	snprintf(buf, buf_len, "%d", read_uint32(data));
}

static void decode_string(const struct struct_field *field, void *data,
                          char *buf, size_t buf_len)
{
	size_t cnt = buf_len;

	if (field->param < cnt) {
		cnt = field->param;
	}

	memcpy(buf, data, cnt);
	if (buf_len > cnt) {
		buf[cnt] = '\0';
	}
}

static size_t field_size_string(const struct struct_field *field)
{
	return field->param;
};

const struct struct_field_type
	field_type_int8 = {"int8", decode_int8, field_size_1},
	field_type_int16 = {"int16", decode_int16, field_size_2},
	field_type_int32 = {"int32", decode_int32, field_size_4},
	field_type_uint8 = {"uint8", decode_uint8, field_size_1},
	field_type_uint16 = {"uint16", decode_uint16, field_size_2},
	field_type_uint32 = {"uint32", decode_uint32, field_size_4},
	field_type_string = {"string", decode_string, field_size_string};

const struct struct_field vertex_struct_fields[] = {
	{&field_type_int16, "x"},
	{&field_type_int16, "y"},
};

const struct struct_type vertex_struct = {
	"vertex",
	vertex_struct_fields,
	arrlen(vertex_struct_fields),
};

const struct struct_field linedef_struct_fields[] = {
	{&field_type_uint16, "v1"},
	{&field_type_uint16, "v2"},
	{&field_type_uint16, "flags"},
	{&field_type_uint16, "special"},
	{&field_type_uint16, "tag"},
	{&field_type_uint16, "sidenum1"},
	{&field_type_uint16, "sidenum2"},
};

const struct struct_type linedef_struct = {
	"linedef",
	linedef_struct_fields,
	arrlen(linedef_struct_fields),
};

const struct struct_field sidedef_struct_fields[] = {
	{&field_type_int16, "textureoffset"},
	{&field_type_int16, "rowoffset"},
	{&field_type_string, "toptexture", 8},
	{&field_type_string, "bottomtexture", 8},
	{&field_type_string, "midtexture", 8},
	{&field_type_uint16, "sector"},
};

const struct struct_type sidedef_struct = {
	"sidedef",
	sidedef_struct_fields,
	arrlen(sidedef_struct_fields),
};

const struct struct_field sector_struct_fields[] = {
	{&field_type_int16, "floorheight"},
	{&field_type_int16, "ceilingheight"},
	{&field_type_string, "floorpic", 8},
	{&field_type_string, "ceilingpic", 8},
	{&field_type_uint16, "lightlevel"},
	{&field_type_uint16, "special"},
	{&field_type_uint16, "tag"},
};

const struct struct_type sector_struct = {
	"sector",
	sector_struct_fields,
	arrlen(sector_struct_fields),
};

size_t struct_length(const struct struct_type *s)
{
	const struct struct_field *f;
	size_t result = 0;
	int i;

	for (i = 0; i < s->num_fields; i++) {
		f = &s->fields[i];
		result += f->field_type->size(f);
	}

	return result;
}

#ifdef TEST

void print_struct(const struct struct_type *s, uint8_t *data)
{
	const struct struct_field *f;
	char prtbuf[16];
	int i;

	for (i = 0; i < s->num_fields; i++) {
		f = &s->fields[i];
		f->field_type->decode(f, data, prtbuf, sizeof(prtbuf));
		printf("  %s: %s\n", f->name, prtbuf);
		data += f->field_type->size(f);
	}
}

// Test program that prints the contents of a SIDEDEFS lump.
int main(int argc, char *argv[])
{
	FILE *fs = fopen(argv[1], "rb");
	size_t struct_sz = struct_length(&sidedef_struct);
	uint8_t buf[32];
	int i;

	for (i = 0;; i++) {
		if (fread(buf, struct_sz, 1, fs) < 1) {
			break;
		}
		printf("%d:\n", i);
		print_struct(&sidedef_struct, buf);
	}

	fclose(fs);
}
#endif

