//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// POSIX compatibility functions for Windows build

#include "compat.h"

#ifdef _WIN32

#include <stdio.h>
#include <string.h>

ssize_t readlink(const char *restrict pathname, char *restrict buf,
                 size_t bufsiz)
{
	FILE *fs;
	size_t cnt;

	fs = fopen(pathname, "rb");
	if (fs == NULL) {
		return -1;
	}

	cnt = fread(buf, 1, bufsiz, fs);
	fclose(fs);

	if (cnt == 0) {
		return -1;
	}

	return cnt;
}

int symlink(const char *target, const char *linkpath)
{
	FILE *fs;
	size_t cnt;

	fs = fopen(linkpath, "wb");
	if (fs == NULL) {
		return -1;
	}

	cnt = fwrite(target, 1, strlen(target), fs);

	fclose(fs);

	return cnt == strlen(target) ? 0 : -1;
}

#endif
