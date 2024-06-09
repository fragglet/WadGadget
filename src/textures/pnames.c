//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "conv/error.h"
#include "fs/vfile.h"
#include "fs/vfs.h"

#include "textures/textures.h"

void TX_FreePnames(struct pnames *pnames)
{
	free(pnames->pnames);
	free(pnames);
}

struct pnames *TX_UnmarshalPnames(VFILE *f)
{
	struct pnames *pnames = checked_calloc(1, sizeof(struct pnames));
	uint32_t cnt, names_read;

	if (vfread(&cnt, sizeof(uint32_t), 1, f) != 1) {
		ConversionError("Failed to read 4 byte lump header.");
		free(pnames);
		vfclose(f);
		return NULL;
	}

	SwapLE32(&cnt);
	pnames->num_pnames = cnt;
	pnames->pnames = checked_calloc(cnt, sizeof(pname));

	names_read = vfread(pnames->pnames, sizeof(pname), cnt, f);
	if (names_read != cnt) {
		ConversionError("Lump too short: only %d/%d names read.",
		                names_read, cnt);
		TX_FreePnames(pnames);
		vfclose(f);
		return NULL;
	}

	vfclose(f);
	return pnames;
}

VFILE *TX_MarshalPnames(struct pnames *pn)
{
	VFILE *result = vfopenmem(NULL, 0);
	uint32_t cnt = pn->num_pnames;

	SwapLE32(&cnt);
	if (vfwrite(&cnt, sizeof(uint32_t), 1, result) != 1) {
		ConversionError("Failed to write header.");
		vfclose(result);
		return NULL;
	}

	if (vfwrite(pn->pnames, sizeof(pname),
	            pn->num_pnames, result) != pn->num_pnames) {
		ConversionError("Failed to write names.");
		vfclose(result);
		return NULL;
	}
	vfseek(result, 0, SEEK_SET);
	return result;
}

int TX_AppendPname(struct pnames *pn, const char *name)
{
	int result;

	pn->pnames = checked_realloc(pn->pnames,
	                             sizeof(pname) * (pn->num_pnames + 1));
	strncpy(pn->pnames[pn->num_pnames], name, 8);
	result = pn->num_pnames;
	++pn->num_pnames;
	TX_RenamePname(pn, result, name);
	++pn->modified_count;
	return result;
}

int TX_GetPnameIndex(struct pnames *pn, const char *name)
{
	int i;

	for (i = 0; i < pn->num_pnames; i++) {
		if (!strncasecmp(pn->pnames[i], name, 8)) {
			return i;
		}
	}

	return -1;
}

void TX_RemovePname(struct pnames *pn, unsigned int idx)
{
	assert(idx < pn->num_pnames);

	memmove(&pn->pnames[idx], &pn->pnames[idx + 1],
	        sizeof(pname) * (pn->num_pnames - idx - 1));
	--pn->num_pnames;
	++pn->modified_count;
}

void TX_RenamePname(struct pnames *pn, unsigned int idx, const char *name)
{
	char *namedest;
	int i;

	assert(idx < pn->num_pnames);

	namedest = pn->pnames[idx];
	for (i = 0; i < 8; i++) {
		namedest[i] = toupper(name[i]);
		if (namedest[i] == '\0') {
			break;
		}
	}

	++pn->modified_count;
}
