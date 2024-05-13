//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdint.h>
#include <assert.h>

#define SHIFT_KEY_F(n) KEY_F(n + 12)

#define arrlen(x) (sizeof(x) / sizeof(*(x)))

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))

#define or_if_null(x, y) ((x) != NULL ? (x) : (y))

static inline void *check_allocation_result(void *x)
{
	assert(x != NULL);
	return x;
}

#define checked_malloc(size) \
    check_allocation_result(malloc(size))

#define checked_calloc(nmemb, size) \
    check_allocation_result(calloc(nmemb, size))

#define checked_realloc(ptr, size) \
    check_allocation_result(realloc(ptr, size))

#define checked_strdup(s) \
    check_allocation_result(strdup(s))

// Endianness conversions.
static inline void SwapLE32(void *i)
{
	uint8_t *b = i;
	uint32_t *i32 = i;
	*i32 = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];
}

static inline void SwapBE32(void *i)
{
	uint8_t *b = i;
	uint32_t *i32 = i;
	*i32 = (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static inline void SwapLE16(void *i)
{
	uint8_t *b = i;
	uint16_t *i16 = i;
	*i16 = (b[1] << 8) | b[0];
}

static inline void SwapBE16(void *i)
{
	uint8_t *b = i;
	uint16_t *i16 = i;
	*i16 = (b[0] << 8) | b[1];
}

