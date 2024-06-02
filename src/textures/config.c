//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// Functions related to deutex-format texture config files.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "common.h"
#include "fs/vfile.h"
#include "fs/vfs.h"

#include "textures/textures.h"

#define TEXTURE_CONFIG_HEADER "; deutex format texture lump configuration\n\n"
#define PNAMES_CONFIG_HEADER "; patch names lump configuration\n\n"

static bool CheckTextureConfig(struct textures *txs, struct pnames *pn)
{
	int i, j;

	for (i = 0; i < txs->num_textures; i++) {
		struct texture *t = txs->textures[i];

		for (j = 0; j < t->patchcount; j++) {
			if (t->patches[j].patch >= pn->num_pnames) {
				return false;
			}
		}
	}

	return true;
}

VFILE *TX_FormatTexturesConfig(struct textures *txs, struct pnames *pn)
{
	struct texture *t;
	VFILE *result = vfopenmem(NULL, 0);
	char buf[32];
	int i, j;

	if (!CheckTextureConfig(txs, pn)) {
		vfclose(result);
		return NULL;
	}

	assert(vfwrite(TEXTURE_CONFIG_HEADER,
	               strlen(TEXTURE_CONFIG_HEADER), 1, result) == 1);

	for (i = 0; i < txs->num_textures; i++) {
		t = txs->textures[i];

		snprintf(buf, sizeof(buf), "%-8.8s %8d %8d\n", t->name,
		         t->width, t->height);
		assert(vfwrite(buf, strlen(buf), 1, result) == 1);

		for (j = 0; j < t->patchcount; j++) {
			const struct patch *p = &t->patches[j];
			snprintf(buf, sizeof(buf), "* %-8.8s %6d %8d\n",
			         pn->pnames[p->patch], p->originx,
			         p->originy);
			assert(vfwrite(buf, strlen(buf), 1, result) == 1);
		}
	}

	assert(vfseek(result, 0, SEEK_SET) == 0);

	return result;
}

VFILE *TX_FormatPnamesConfig(struct pnames *p)
{
	VFILE *result = vfopenmem(NULL, 0);
	char buf[32];
	int i;

	assert(vfwrite(PNAMES_CONFIG_HEADER,
	               strlen(PNAMES_CONFIG_HEADER), 1, result) == 1);

	for (i = 0; i < p->num_pnames; i++) {
		snprintf(buf, sizeof(buf), "%.8s\n", p->pnames[i]);
		assert(vfwrite(buf, strlen(buf), 1, result) == 1);
	}

	assert(vfseek(result, 0, SEEK_SET) == 0);

	return result;
}

static char *ReadLine(uint8_t *buf, size_t buf_len, unsigned int *offset)
{
	char *result, *p;
	unsigned int start, len;

	if (*offset >= buf_len) {
		return NULL;
	}

	// Skip leading spaces.
	while (*offset < buf_len && buf[*offset] != '\n'
	    && isspace(buf[*offset])) {
		++*offset;
	}

	start = *offset;
	while (*offset < buf_len && buf[*offset] != '\n') {
		++*offset;
	}

	len = *offset - start;
	result = checked_calloc(len + 1, 1);
	memcpy(result, &buf[start], len);
	result[len] = '\0';
	++*offset;

	// Strip out comments.
	p = strchr(result, ';');
	if (p != NULL) {
		*p = '\0';
	}

	return result;
}

static int ScanLine(char *line, const char *fmt, char *name,
                    int *x, int *y)
{
	int nfields;

	*x = 0;
	*y = 0;
	nfields = sscanf(line, fmt, name, x, y);
	if (nfields < 1) {
		return 0;
	}

	// We only allow 8 characters, but parse up to 9 to see if
	// the limit was exceeded.
	name[9] = '\0';
	if (strlen(name) > 8) {
		return 0;
	}

	return nfields;
}

static bool MaybeAddTexture(struct textures *txs, char *line)
{
	struct texture *t;
	char namebuf[10];
	int w, h, n;

	n = ScanLine(line, "%9s %d %d", namebuf, &w, &h);
	if (n < 3) {
		return false;
	}

	t = TX_AllocTexture(0);
	memcpy(t->name, namebuf, 8);
	t->width = w;
	t->height = h;
	t->patchcount = 0;

	txs->textures = checked_realloc(txs->textures,
		(txs->num_textures + 1) * sizeof(struct texture *));
	txs->textures[txs->num_textures] = t;
	++txs->num_textures;

	return true;
}

static bool MaybeAddPatch(struct textures *txs, char *line,
                          struct pnames *pnames)
{
	char namebuf[10];
	struct texture **t;
	struct patch p;
	int x, y, n;

	if (txs->num_textures <= 0) {
		return false;
	}

	n = ScanLine(line, "* %9s %d %d", namebuf, &x, &y);
	// As long as we have the name, we can append the patch
	// (X/Y offsets are assumed to be zero)
	if (n < 1) {
		return false;
	}

	p.originx = x;
	p.originy = y;
	n = TX_GetPnameIndex(pnames, namebuf);
	if (n < 0) {
		n = TX_AppendPname(pnames, namebuf);
	}
	p.patch = n;
	p.stepdir = 0;
	p.colormap = 0;

	t = &txs->textures[txs->num_textures - 1];
	*t = TX_AddPatch(*t, &p);

	return true;
}

static struct textures *ParseTextureConfig(uint8_t *buf, size_t buf_len,
                                           struct pnames *pnames)
{
	struct textures *result;
	unsigned int offset = 0;
	bool fail;

	result = calloc(1, sizeof(struct textures));

	for (;;) {
		char *line = ReadLine(buf, buf_len, &offset);
		if (line == NULL) {
			break;
		}

		fail = strlen(line) > 0
		    && !MaybeAddPatch(result, line, pnames)
		    && !MaybeAddTexture(result, line);
		free(line);

		if (fail) {
			// error
			TX_FreeTextures(result);
			return NULL;
		}
	}

	TX_AddSerialNos(result);

	return result;
}

struct textures *TX_ParseTextureConfig(VFILE *input, struct pnames *pn)
{
	VFILE *sink = vfopenmem(NULL, 0);
	struct textures *result;
	void *lump;
	size_t lump_len;

	vfcopy(input, sink);
	vfclose(input);

	if (!vfgetbuf(sink, &lump, &lump_len)) {
		vfclose(sink);
		return NULL;
	}

	result = ParseTextureConfig(lump, lump_len, pn);
	vfclose(sink);

	return result;
}

static struct pnames *ParsePnamesConfig(uint8_t *buf, size_t buf_len)
{
	struct pnames *result;
	unsigned int offset = 0;

	result = calloc(1, sizeof(struct pnames));
	result->pnames = NULL;
	result->num_pnames = 0;

	for (;;) {
		char *line = ReadLine(buf, buf_len, &offset), *p;
		if (line == NULL) {
			break;
		}

		// Strip trailing spaces.
		p = line + strlen(line);
		while (p > line) {
			--p;
			if (!isspace(*p)) {
				break;
			}
			*p = '\0';
		}

		if (strlen(p) > 8) {
			TX_FreePnames(result);
			return NULL;
		} else if (strlen(p) > 0) {
			TX_AppendPname(result, line);
		}

		free(line);
	}

	result->modified = false;

	return result;
}

struct pnames *TX_ParsePnamesConfig(VFILE *input)
{
	VFILE *sink = vfopenmem(NULL, 0);
	struct pnames *result;
	void *cfg;
	size_t cfg_len;

	vfcopy(input, sink);
	vfclose(input);

	if (!vfgetbuf(sink, &cfg, &cfg_len)) {
		return NULL;
	}

	result = ParsePnamesConfig(cfg, cfg_len);
	vfclose(sink);
	return result;
}
