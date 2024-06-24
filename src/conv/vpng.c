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
// Basic libpng wrapper functions for reading and writing PNG files
// through the VFILE interface.

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "fs/vfile.h"
#include "conv/error.h"
#include "conv/vpng.h"

static jmp_buf libpng_abort_jump;

static void ErrorCallback(png_structp p, png_const_charp s)
{
	ConversionError("%s", s);
	longjmp(libpng_abort_jump, 1);
}

static void WarningCallback(png_structp p, png_const_charp s)
{
	fprintf(stderr, "libpng warning: %s\n", s);
}

static void PngReadCallback(png_structp ppng, png_bytep buf, size_t len)
{
	VFILE *vf = png_get_io_ptr(ppng);
	int result;

	memset(buf, 0, len);
	result = vfread(buf, 1, len, vf);
	if (result == 0) {
		png_error(ppng, "end of file reached");
	} else if (result < 0) {
		png_error(ppng, "read error");
	}
}

static void PngWriteCallback(png_structp ppng, png_bytep buf, size_t len)
{
	VFILE *vf = png_get_io_ptr(ppng);
	vfwrite(buf, 1, len, vf);
}

static void PngFlushCallback(png_structp ppng)
{
	// no-op
}

void V_ClosePNG(struct png_context *ctx)
{
	if (ctx->write) {
		png_destroy_write_struct(&ctx->ppng, &ctx->pinfo);
	} else {
		png_destroy_read_struct(&ctx->ppng, &ctx->pinfo, NULL);
	}
}

bool V_OpenPNGRead(struct png_context *ctx, VFILE *input)
{
	ctx->ppng = NULL;
	ctx->pinfo = NULL;
	ctx->write = false;

	if (setjmp(libpng_abort_jump) != 0) {
		ConversionError("Error when parsing PNG file");
		V_ClosePNG(ctx);
		return NULL;
	}

	ctx->ppng = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                   ErrorCallback, WarningCallback);
	if (ctx->ppng == NULL) {
		ConversionError("Failed to open PNG file");
		return false;
	}

	ctx->pinfo = png_create_info_struct(ctx->ppng);
	if (ctx->pinfo == NULL) {
		ConversionError("Failed to create PNG info struct");
		V_ClosePNG(ctx);
		return false;
	}

	png_set_read_fn(ctx->ppng, input, PngReadCallback);
	return true;
}

VFILE *V_OpenPNGWrite(struct png_context *ctx)
{
	VFILE *result;

	ctx->ppng = NULL;
	ctx->pinfo = NULL;
	ctx->write = true;

	if (setjmp(libpng_abort_jump) != 0) {
		ConversionError("Error when writing PNG file");
		V_ClosePNG(ctx);
		return NULL;
	}

	ctx->ppng = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
	                                    ErrorCallback, WarningCallback);
	if (!ctx->ppng) {
		ConversionError("Failed to create PNG write struct");
		return NULL;
	}

	ctx->pinfo = png_create_info_struct(ctx->ppng);
	if (!ctx->pinfo) {
		ConversionError("Failed to create PNG info struct");
		V_ClosePNG(ctx);
		return NULL;
	}

	result = vfopenmem(NULL, 0);
	png_set_write_fn(ctx->ppng, result, PngWriteCallback, PngFlushCallback);

	return result;
}
