//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef STRINGS_H_INCLUDED
#define STRINGS_H_INCLUDED

#include <stdarg.h>

char *StringDuplicate(const char *orig);
int StringCopy(char *dest, const char *src, size_t dest_size);
int StringConcat(char *dest, const char *src, size_t dest_size);
int StringHasPrefix(const char *s, const char *prefix);
int StringHasSuffix(const char *s, const char *suffix);
const char *StrCaseStr(const char *haystack, const char *needle);
char *StringReplace(const char *haystack, const char *needle);
char *StringJoin(const char *sep, const char *s, ...);
int VStringPrintf(char *buf, size_t buf_len, const char *s, va_list args);
int StringPrintf(char *buf, size_t buf_len, const char *s, ...);

char *PathDirName(const char *path);
const char *PathBaseName(const char *path);
char *PathSanitize(const char *filename);
char *MakeDirectories(const char *first, ...);

#endif /* #ifndef STRINGS_H_INCLUDED */

