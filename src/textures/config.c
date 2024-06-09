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
#include "conv/error.h"
#include "fs/vfile.h"
#include "fs/vfs.h"

#include "textures/textures.h"

#define TEXTURE_CONFIG_HEADER "; deutex format texture lump configuration\n"
#define PNAMES_CONFIG_HEADER "; patch names lump configuration\n\n"

static bool CheckTextureConfig(struct textures *txs, struct pnames *pn)
{
	int i, j;

	for (i = 0; i < txs->num_textures; i++) {
		struct texture *t = txs->textures[i];

		for (j = 0; j < t->patchcount; j++) {
			if (t->patches[j].patch >= pn->num_pnames) {
				ConversionError(
					"Texture %.8s patch #%d has invalid "
					"PNAMES index %d >= %d", t->name, j,
					t->patches[j].patch, pn->num_pnames);
				return false;
			}
		}
	}

	return true;
}

VFILE *TX_FormatTexturesConfig(struct textures *txs, struct pnames *pn,
                               const char *comment)
{
	struct texture *t;
	VFILE *result = vfopenmem(NULL, 0);
	char buf[32];
	int i, j;

	if (!CheckTextureConfig(txs, pn)) {
		ConversionError("Error formatting texture config");
		vfclose(result);
		return NULL;
	}

	assert(vfwrite(TEXTURE_CONFIG_HEADER,
	               strlen(TEXTURE_CONFIG_HEADER), 1, result) == 1);

	if (comment != NULL) {
		assert(vfwrite("; ", 2, 1, result) == 1);
		assert(vfwrite(comment, strlen(comment), 1, result) == 1);
		assert(vfwrite("\n", 1, 1, result) == 1);
	}

	assert(vfwrite("\n", 1, 1, result) == 1);

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

static void StripTrailingSpaces(char *line)
{
	char *p = line + strlen(line);
	while (p > line) {
		--p;
		if (!isspace(*p)) {
			break;
		}
		*p = '\0';
	}
}

static void StringUpper(char *line)
{
	char *p;
	for (p = line; *p != '\0'; ++p) {
		*p = toupper(*p);
	}
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
	p = strchr(result, '#');
	if (p != NULL) {
		*p = '\0';
	}

	StripTrailingSpaces(result);
	StringUpper(result);

	return result;
}

static int ScanLine(char *line, const char *fmt, char *name,
                    int *x, int *y, int *token_cols, int *error_col)
{
	int nfields;

	*x = 0;
	*y = 0;
	nfields = sscanf(line, fmt, &token_cols[0], name, &token_cols[1], x,
	                 &token_cols[2], y, &token_cols[3]);
	if (nfields < 1) {
		return 0;
	}

	// We only allow 8 characters, but parse up to 9 to see if
	// the limit was exceeded.
	name[9] = '\0';
	if (strlen(name) > 8) {
		ConversionError("Name contains more than 8 characters");
		*error_col = token_cols[0] + 8;
		return -1;
	}

	// Should be no junk left over on the end of line
	if (token_cols[nfields] < strlen(line)) {
		ConversionError("Line contains trailing characters");
		*error_col = token_cols[nfields];
		return -1;
	}

	return nfields;
}

enum parse_result { ERROR, NO_MATCH, MATCH };

static enum parse_result MaybeAddTexture(struct textures *txs, char *line,
                                         int *error_col)
{
	struct texture *t;
	int token_cols[4];
	char namebuf[10];
	int w, h, n;

	n = ScanLine(line, "%n%9s %n%d %n%d %n", namebuf, &w, &h,
	             token_cols, error_col);
	if (n < 0) {
		return ERROR;
	}
	if (n < 3) {
		return NO_MATCH;
	}

	if (w < 1) {
		ConversionError("Texture must have positive width");
		*error_col = token_cols[1];
		return ERROR;
	}
	if (h < 1) {
		ConversionError("Texture must have positive height");
		*error_col = token_cols[2];
		return ERROR;
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

	return MATCH;
}

static enum parse_result MaybeAddPatch(struct textures *txs, char *line,
                                       struct pnames *pnames, int *error_col)
{
	char namebuf[10];
	int token_cols[4];
	struct texture **t;
	struct patch p;
	int x, y, n;

	n = ScanLine(line, "* %n%9s %n%d %n%d %n", namebuf, &x, &y,
	             token_cols, error_col);
	if (n < 0) {
		return ERROR;
	}

	// As long as we have the name, we can append the patch
	// (X/Y offsets are assumed to be zero)
	if (n < 1) {
		return NO_MATCH;
	}

	if (txs->num_textures <= 0) {
		ConversionError("Must start a texture definition first");
		*error_col = 0;
		return ERROR;
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

	return MATCH;
}

static struct textures *ParseTextureConfig(uint8_t *buf, size_t buf_len,
                                           struct pnames *pnames)
{
	struct textures *result;
	char *line = NULL, *highlight = NULL;
	unsigned int offset = 0;
	int lineno = 0, m, error_col;

	result = calloc(1, sizeof(struct textures));

	for (;;) {
		free(line);
		line = ReadLine(buf, buf_len, &offset);
		if (line == NULL) {
			break;
		}

		++lineno;
		error_col = -1;
		if (strlen(line) == 0) {
			continue;
		}

		m = MaybeAddPatch(result, line, pnames, &error_col);
		if (m == ERROR) {
			goto fail;
		} else if (m == MATCH) {
			continue;
		}

		m = MaybeAddTexture(result, line, &error_col);
		if (m == ERROR) {
			goto fail;
		} else if (m == MATCH) {
			continue;
		}

		goto fail;
	}

	TX_AddSerialNos(result);
	free(line);

	return result;

fail:
	if (error_col >= 0) {
		highlight = checked_calloc(error_col + 3, 1);
		memset(highlight, ' ', error_col);
		highlight[error_col] = '^';
		highlight[error_col + 1] = '\n';
		highlight[error_col + 2] = '\0';
	} else {
		highlight = checked_strdup("");
	}
	ConversionError("Syntax error on line #%d:\n\n%s\n%s",
	                lineno, line, highlight);
	free(line);
	free(highlight);
	TX_FreeTextures(result);
	return NULL;
}

struct textures *TX_ParseTextureConfig(VFILE *input, struct pnames *pn)
{
	struct textures *result;
	void *lump;
	size_t lump_len;

	lump = vfreadall(input, &lump_len);
	vfclose(input);
	result = ParseTextureConfig(lump, lump_len, pn);
	free(lump);

	result->modified_count = 0;

	return result;
}

static struct pnames *ParsePnamesConfig(uint8_t *buf, size_t buf_len)
{
	struct pnames *result;
	unsigned int offset = 0, lineno = 0;

	result = calloc(1, sizeof(struct pnames));
	result->pnames = NULL;
	result->num_pnames = 0;

	for (;;) {
		char *line = ReadLine(buf, buf_len, &offset);
		if (line == NULL) {
			break;
		}

		++lineno;
		if (strlen(line) > 8) {
			ConversionError("Patch name on line #%d exceeds 8 "
			                "characters:\n\n%s\n        ^",
			                lineno, line);
			TX_FreePnames(result);
			return NULL;
		} else if (strlen(line) > 0) {
			TX_AppendPname(result, line);
		}

		free(line);
	}

	result->modified_count = 0;

	return result;
}

struct pnames *TX_ParsePnamesConfig(VFILE *input)
{
	struct pnames *result;
	void *cfg;
	size_t cfg_len;

	cfg = vfreadall(input, &cfg_len);
	vfclose(input);
	result = ParsePnamesConfig(cfg, cfg_len);
	free(cfg);
	return result;
}
