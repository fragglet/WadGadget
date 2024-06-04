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

static bool have_error;
static char conversion_error[128];

void ClearConversionErrors(void)
{
	strncpy(conversion_error, "", sizeof(conversion_error));
	have_error = false;
}

void ConversionError(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf(conversion_error, sizeof(conversion_error), fmt, args);
	va_end(args);

	have_error = true;
}

const char *GetConversionError(void)
{
	return conversion_error;
}
