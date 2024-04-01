
#ifndef INCLUDED_STRUCT_H
#define INCLUDED_STRUCT_H

struct struct_field;

struct struct_field_type {
	const char *type_name;
	void (*decode)(const struct struct_field *field, void *data,
	               char *buf, size_t buf_len);
	size_t (*size)(const struct struct_field *field);
};

struct struct_field {
	const struct struct_field_type *field_type;
	const char *name;
	int param;
};

struct struct_type {
	const char *name;
	const struct struct_field *fields;
	int num_fields;
};

extern const struct struct_field_type field_type_int8;
extern const struct struct_field_type field_type_int16;
extern const struct struct_field_type field_type_int32;
extern const struct struct_field_type field_type_uint8;
extern const struct struct_field_type field_type_uint16;
extern const struct struct_field_type field_type_uint32;
extern const struct struct_field_type field_type_string;

#endif /* #ifndef INCLUDED_STRUCT_H */

