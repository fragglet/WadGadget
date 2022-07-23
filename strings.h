#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

char *StringDuplicate(const char *orig);
int StringCopy(char *dest, const char *src, size_t dest_size);
int StringConcat(char *dest, const char *src, size_t dest_size);
int StringHasPrefix(const char *s, const char *prefix);
int StringHasSuffix(const char *s, const char *suffix);
const char *StrCaseStr(const char *haystack, const char *needle);
char *StringReplace(const char *haystack, const char *needle,;
char *StringJoin(const char *sep, const char *s, ...);
int VStringPrintf(char *buf, size_t buf_len, const char *s, va_list args);
int StringPrintf(char *buf, size_t buf_len, const char *s, ...);

char *PathDirName(const char *path);
const char *PathBaseName(const char *path);
char *PathSanitize(const char *filename);

#endif /* #ifndef STRINGS_H_INCLUDED */

