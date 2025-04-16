//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "conv/netpbm.h"

#include <assert.h>
#include <stdlib.h>

#include "common.h"
#include "conv/process.h"
#include "stringlib.h"

#define TRANSPARENT "rgb:00/ff/ff" /* cyan */

static const struct {
	const char *extension;
	const char *converter;
} file_types[] = {
    {".pcx",  "pcxtoppm" },
    {".lbm",  "ilbmtoppm"},
    {".gif",  "giftopnm" },
    {".bmp",  "bmptopnm" },
    {".ppm",  NULL       },
    {".tif",  "tifftopnm"},
    {".tiff", "tifftopnm"},
    {".xpm",  "xpmtoppm" },
};

static bool NetpbmInstalled(void)
{
	return system("pnmtopng --version >/dev/null 2>&1") == 0;
}

bool NetpbmFileTypeSupported(const char *filename)
{
	int i;

	for (i = 0; i < arrlen(file_types); i++) {
		if (StringHasSuffix(filename, file_types[i].extension)) {
			return NetpbmInstalled();
		}
	}

	return false;
}

VFILE *NetpbmConvertToPNG(VFILE *input, const char *filename)
{
	const char *to_pnm[] = {"-", NULL};
	const char *to_png[] = {"pnmtopng", "-transparent=" TRANSPARENT, NULL};
	int i;

	for (i = 0; i < arrlen(file_types); i++) {
		if (StringHasSuffix(filename, file_types[i].extension)) {
			break;
		}
	}

	assert(i < arrlen(file_types));

	if (file_types[i].converter != NULL) {
		to_pnm[0] = file_types[i].converter;
		input = SpawnSubprocessFilter(input, to_pnm, true);
	}

	// We usually discard any errors from pnmtopng as we can assume the
	// output from the first converter will always be valid; we only report
	// its error output if we're converting from .ppm and there is no first
	// converter:
	return SpawnSubprocessFilter(input, to_png,
	                             file_types[i].converter == NULL);
}
