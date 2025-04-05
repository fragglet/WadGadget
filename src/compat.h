//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifdef _WIN32

#include <stdlib.h>

ssize_t readlink(const char *restrict pathname, char *restrict buf,
                 size_t bufsiz);
int symlink(const char *target, const char *linkpath);

// The win32 version of mkdir() only takes a single argument:
#define mkdir(path, perms) ((mkdir)(path))

// win32 has no fsync() but _commit() appears to do the same thing:
#define fsync _commit

#endif  /* #ifdef _WIN32 */

