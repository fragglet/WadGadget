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
#include <stdarg.h>
#include <stdbool.h>

#include "conv/error.h"

#define MAX_ERROR_LEN  256

static bool have_error;
static char conversion_error[MAX_ERROR_LEN];

void ClearConversionErrors(void)
{
	strncpy(conversion_error, "", MAX_ERROR_LEN);
	have_error = false;
}

void ConversionError(char *fmt, ...)
{
	char tmpbuf[MAX_ERROR_LEN];
	va_list args;
	size_t nbytes;

	// If ConversionError() is called multiple times, we prepend. This
	// matches the return back up the stack when an error occurs. eg.
	// ConversionError("Texture too short: 4 < 5");
	// ConversionError("When handling texture 'FOO'");
	// ConversionError("Error when importing lump 'TEXTURE1'");
	strncpy(tmpbuf, conversion_error, MAX_ERROR_LEN);

	va_start(args, fmt);
	nbytes = vsnprintf(conversion_error, MAX_ERROR_LEN, fmt, args);
	va_end(args);

	snprintf(conversion_error + nbytes, MAX_ERROR_LEN - nbytes,
	         "\n%s", tmpbuf);

	have_error = true;
}

const char *GetConversionError(void)
{
	return conversion_error;
}
