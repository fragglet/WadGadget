#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#ifdef _WIN32
#define DIR_SEPARATOR "\\"
#else
#define DIR_SEPARATOR "/"
#endif

static void *CheckAllocation(void *x)
{
	if (x == NULL) {
		fprintf(stderr, "Memory allocation failed.\n");
		abort();
	}
	return x;
}

char *StringDuplicate(const char *orig)
{
	return CheckAllocation(strdup(orig));
}

// Safe string copy function that works like OpenBSD's strlcpy().
// Returns non-zero if the string was not truncated.
int StringCopy(char *dest, const char *src, size_t dest_size)
{
	size_t len;

	if (dest_size < 1) {
		return 0;
	}

	dest[dest_size - 1] = '\0';
	strncpy(dest, src, dest_size - 1);
	len = strlen(dest);
	return src[len] == '\0';
}

// Safe string concat function that works like OpenBSD's strlcat().
// Returns non-zero if string not truncated.
int StringConcat(char *dest, const char *src, size_t dest_size)
{
	size_t offset;

	offset = strlen(dest);
	if (offset > dest_size) {
		offset = dest_size;
	}

	return StringCopy(dest + offset, src, dest_size - offset);
}

// Returns non-zero if 's' begins with the specified prefix.
int StringHasPrefix(const char *s, const char *prefix)
{
	return strlen(s) >= strlen(prefix)
	    && strncmp(s, prefix, strlen(prefix)) == 0;
}

// Returns non-zero if 's' ends with the specified suffix.
int StringHasSuffix(const char *s, const char *suffix)
{
	return strlen(s) >= strlen(suffix)
	    && strcmp(s + strlen(s) - strlen(suffix), suffix) == 0;
}

// Case-insensitive version of strstr()
const char *StrCaseStr(const char *haystack, const char *needle)
{
    unsigned int haystack_len;
    unsigned int needle_len;
    unsigned int len;
    unsigned int i;

    haystack_len = strlen(haystack);
    needle_len = strlen(needle);

    if (haystack_len < needle_len)
    {
        return NULL;
    }

    len = haystack_len - needle_len;

    for (i = 0; i <= len; ++i)
    {
        if (!strncasecmp(haystack + i, needle, needle_len))
        {
            return haystack + i;
        }
    }

    return NULL;
}


// Returns a copy of `haystack` with `needle` replaced by `replacement`.
char *StringReplace(const char *haystack, const char *needle,
                    const char *replacement)
{
	char *result, *dst;
	const char *p;
	size_t needle_len = strlen(needle);
	size_t result_len, dst_len;
	size_t replacement_len = strlen(replacement);

	// Iterate through occurrences of 'needle' and calculate the size of
	// the new string.
	result_len = strlen(haystack) + 1;
	p = haystack;

	for (;;) {
		p = strstr(p, needle);
		if (p == NULL) {
			break;
		}

		p += needle_len;
		result_len += replacement_len - needle_len;
	}

	// Construct new string.
	result = CheckAllocation(malloc(result_len));
	dst = result; dst_len = result_len;
	p = haystack;

	while (*p != '\0') {
		if (!strncmp(p, needle, needle_len)) {
			StringCopy(dst, replacement, dst_len);
			p += needle_len;
			dst += strlen(replacement);
			dst_len -= strlen(replacement);
		} else {
			*dst = *p;
			++dst; --dst_len;
			++p;
		}
	}

	*dst = '\0';

	return result;
}

// Return a newly-malloced string with all the strings given as arguments
// concatenated together.
char *StringJoin(const char *sep, const char *s, ...)
{
	char *result, *r;
	const char *v;
	va_list args;
	size_t result_len = strlen(s) + 1, sep_len = strlen(sep), r_len;

	va_start(args, s);
	for (;;) {
		v = va_arg(args, const char *);
		if (v == NULL) {
			break;
		}

		result_len += strlen(v) + sep_len;
	}
	va_end(args);

	result = CheckAllocation(malloc(result_len));
	StringCopy(result, s, result_len);

	va_start(args, s);
	r = result; r_len = result_len;
	for (;;) {
		size_t v_len;
		v = va_arg(args, const char *);
		if (v == NULL) {
			break;
		}

		StringConcat(r, sep, result_len);
		r += sep_len; r_len -= sep_len;

		v_len = strlen(v);
		StringConcat(r, v, result_len);
		r += v_len; r_len -= v_len;
	}
	va_end(args);

	return result;
}

// On Windows, vsnprintf() is _vsnprintf().
#ifdef _WIN32
#if _MSC_VER < 1400 /* not needed for Visual Studio 2008 */
#define vsnprintf _vsnprintf
#endif
#endif

// Safe, portable vsnprintf().
int VStringPrintf(char *buf, size_t buf_len, const char *s, va_list args)
{
	int result;

	if (buf_len < 1) {
		return 0;
	}

	// Windows (and other OSes?) has a vsnprintf() that doesn't always
	// append a trailing \0. So we must do it, and write into a buffer
	// that is one byte shorter; otherwise this function is unsafe.
	result = vsnprintf(buf, buf_len, s, args);

	// If truncated, change the final char in the buffer to a \0.
	// A negative result indicates a truncated buffer on Windows.
	if (result < 0 || result >= buf_len) {
		buf[buf_len - 1] = '\0';
		result = buf_len - 1;
	}

	return result;
}

// Safe, portable snprintf().
int StringPrintf(char *buf, size_t buf_len, const char *s, ...)
{
	va_list args;
	int result;
	va_start(args, s);
	result = VStringPrintf(buf, buf_len, s, args);
	va_end(args);
	return result;
}

// Returns the directory portion of the given path, without the trailing
// slash separator character. If no directory is described in the path,
// the string "." is returned. In either case, the result is newly allocated
// and must be freed by the caller after use.
char *PathDirName(const char *path)
{
	char *p, *result;

	p = strrchr(path, DIR_SEPARATOR[0]);
	if (p == NULL) {
		return StringDuplicate(".");
	}

	result = StringDuplicate(path);
	result[p - path] = '\0';
	return result;
}

// Returns the base filename described by the given path (without the
// directory name). The result points inside path and nothing new is
// allocated.
const char *PathBaseName(const char *path)
{
	const char *p;

	p = strrchr(path, DIR_SEPARATOR[0]);
	if (p == NULL) {
		return path;
	}

	return p + 1;
}

// Sanitize path, expanding to an absolute path.
// Path doesn't start with "/" -> implicitly starts from WD/
// "foo////bar" -> "foo/bar"
// "foo/./bar" -> "foo/bar"
// "foo/../bar" -> "bar"
// DOS: path starts with "\" -> "X:\" (X from WD)
//      path starts "X:" (not "X:\") -> WD on X:
