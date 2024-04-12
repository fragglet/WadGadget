//
// Copyright(C) 2022-2024 Simon Howard
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2 of the License, or (at your
// option) any later version. This program is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY; without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <assert.h>

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

