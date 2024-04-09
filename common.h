
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

