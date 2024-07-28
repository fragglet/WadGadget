
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"
#include "stringlib.h"
#include "fs/vfile.h"
#include "palette/palette.h"
#include "conv/graphic.h"
#include "conv/error.h"
#include "conv/vpng.h"

#define PALETTE_SIZE   (256 * 3)

static bool default_palette_loaded = false;
static struct palette default_palette;

struct palette_set *PAL_FromImageFile(VFILE *input)
{
	struct png_context ctx;
	int bit_depth, color_type, ilace_type, comp_type, filter_method;
	int rowstep, x, y, num_palettes;
	png_uint_32 width, height;
	uint8_t *rowbuf;
	struct palette_set *result = NULL;

	if (!V_OpenPNGRead(&ctx, input)) {
		return NULL;
	}

	png_read_info(ctx.ppng, ctx.pinfo);
	png_get_IHDR(ctx.ppng, ctx.pinfo, &width, &height, &bit_depth,
	             &color_type, &ilace_type, &comp_type, &filter_method);

	// check dimensions
	if ((width * height) % 256 != 0) {
		ConversionError("Invalid dimensions for palette: %dx%d = "
		                "%d pixels; should be a multiple of 256",
		                width, height, width * height);
		goto fail;
	}

	num_palettes = width * height / 256;

	// Convert all input files to RGB format.
	png_set_strip_alpha(ctx.ppng);
	if (color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(ctx.ppng);
	}
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_expand_gray_1_2_4_to_8(ctx.ppng);
	}
	if (bit_depth < 8) {
		png_set_packing(ctx.ppng);
	}

	png_read_update_info(ctx.ppng, ctx.pinfo);

	rowstep = png_get_rowbytes(ctx.ppng, ctx.pinfo);
	rowbuf = checked_malloc(rowstep);
	result = checked_calloc(1, sizeof(struct palette_set));
	result->num_palettes = num_palettes;
	result->palettes = checked_calloc(num_palettes, sizeof(struct palette));

	for (y = 0; y < height; ++y) {
		png_read_row(ctx.ppng, rowbuf, NULL);
		for (x = 0; x < width; ++x) {
			int pixel_num = y * width + x;
			int pal_num = pixel_num / 256;
			int color_num = pixel_num % 256;
			struct palette_entry *ent =
				&result->palettes[pal_num].entries[color_num];
			ent->r = rowbuf[x * 3];
			ent->g = rowbuf[x * 3 + 1];
			ent->b = rowbuf[x * 3 + 2];
		}
	}

	free(rowbuf);
	png_read_end(ctx.ppng, NULL);

fail:
	V_ClosePNG(&ctx);
	vfclose(input);
	return result;
}

VFILE *PAL_ToImageFile(struct palette_set *set)
{
	VFILE *result = NULL;
	struct png_context ctx;
	uint8_t *rowbuf;
	int w, h, x, y, idx;

	// For multi-palette sets we put one palette per line, but if it's
	// just one palette then generate a 16x16 square.
	if (set->num_palettes > 1) {
		w = 256;
		h = set->num_palettes;
	} else {
		w = 16;
		h = 16;
	}

	result = V_OpenPNGWrite(&ctx);
	if (result == NULL) {
		goto fail;
	}

	png_set_IHDR(ctx.ppng, ctx.pinfo, w, h, 8, PNG_COLOR_TYPE_RGB,
	             PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
	             PNG_FILTER_TYPE_DEFAULT);
	png_write_info(ctx.ppng, ctx.pinfo);

	rowbuf = checked_calloc(w, 3);
	idx = 0;
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; ++x) {
			int p = idx / 256, e = idx % 256;
			rowbuf[x * 3] = set->palettes[p].entries[e].r;
			rowbuf[x * 3 + 1] = set->palettes[p].entries[e].g;
			rowbuf[x * 3 + 2] = set->palettes[p].entries[e].b;
			++idx;
		}
		png_write_row(ctx.ppng, rowbuf);
	}
	free(rowbuf);

	png_write_end(ctx.ppng, ctx.pinfo);
	vfseek(result, 0, SEEK_SET);

fail:
	V_ClosePNG(&ctx);
	return result;
}

VFILE *PAL_MarshalPaletteSet(const struct palette_set *set)
{
	VFILE *result = vfopenmem(NULL, 0);
	const struct palette *pal;
	int pal_num, color_num;
	uint8_t c[3];

	for (pal_num = 0; pal_num < set->num_palettes; ++pal_num) {
		pal = &set->palettes[pal_num];
		for (color_num = 0; color_num < 256; ++color_num) {
			c[0] = pal->entries[color_num].r;
			c[1] = pal->entries[color_num].g;
			c[2] = pal->entries[color_num].b;
			assert(vfwrite(c, 3, 1, result) == 1);
		}
	}

	vfseek(result, 0, SEEK_SET);
	return result;
}

struct palette_set *PAL_UnmarshalPaletteSet(VFILE *input)
{
	uint8_t *buf, *paldata;
	size_t buf_len, num_palettes;
	int pal_num, color_num;
	struct palette *pal;
	struct palette_set *result;

	buf = vfreadall(input, &buf_len);
	vfclose(input);

	if (buf_len % PALETTE_SIZE != 0) {
		ConversionError("Invalid length for palette lump: %d "
		                "should be a multiple of %d",
		                buf_len, PALETTE_SIZE);
		free(buf);
		return NULL;
	}

	num_palettes = buf_len / PALETTE_SIZE;

	result = checked_calloc(1, sizeof(struct palette_set));
	result->num_palettes = num_palettes;
	result->palettes = checked_calloc(num_palettes, sizeof(struct palette));

	for (pal_num = 0; pal_num < num_palettes; ++pal_num) {
		pal = &result->palettes[pal_num];
		paldata = buf + pal_num * PALETTE_SIZE;
		for (color_num = 0; color_num < 256; ++color_num) {
			pal->entries[color_num].r = paldata[color_num * 3];
			pal->entries[color_num].g = paldata[color_num * 3 + 1];
			pal->entries[color_num].b = paldata[color_num * 3 + 2];
		}
	}

	return result;
}

void PAL_FreePaletteSet(struct palette_set *set)
{
	free(set->palettes);
	free(set);
}

static char *DefaultPointerPath(const char *dir)
{
	return StringJoin("/", dir, "default", NULL);
}

char *PAL_ReadDefaultPointer(void)
{
	const char *dir = PAL_GetPalettesPath();
	char *path = DefaultPointerPath(dir);
	size_t buf_len = 16;
	char *buf = checked_calloc(buf_len, 1);
	ssize_t result;

	for (;;) {
		result = readlink(path, buf, buf_len);
		if (result < 0) {
			free(buf);
			buf = NULL;
			break;
		}
		if (result < buf_len) {
			break;
		}

		buf_len *= 2;
		buf = checked_realloc(buf, buf_len);
	}

	free(path);
	return buf;
}

static void SetDefaultPointer(const char *path, const char *full_name)
{
	char *default_ptr = DefaultPointerPath(path);

	assert(unlink(default_ptr) == 0 || errno == ENOENT);
	assert(symlink(full_name, default_ptr) == 0);
	free(default_ptr);
}

void PAL_SetDefaultPointer(const char *full_name)
{
	SetDefaultPointer(PAL_GetPalettesPath(), full_name);
}

static void LoadDefaultPalette(void)
{
	const char *dir = PAL_GetPalettesPath();
	char *path = DefaultPointerPath(dir);
	VFILE *in = vfwrapfile(fopen(path, "rb"));
	struct palette_set *set;

	assert(in != NULL);
	set = PAL_FromImageFile(in);
	free(path);

	default_palette = set->palettes[0];
	PAL_FreePaletteSet(set);

	default_palette_loaded = true;
}

const struct palette *PAL_DefaultPalette(void)
{
	if (!default_palette_loaded) {
		LoadDefaultPalette();
	}

	return &default_palette;
}

static void WriteDoomPalette(const char *path)
{
	struct palette_set doom_palette_set =
		{(struct palette *) &doom_palette, 1};
	VFILE *out = vfwrapfile(fopen(path, "wb"));
	assert(out != NULL);
	vfcopy(PAL_ToImageFile(&doom_palette_set), out);
	vfclose(out);
}

static bool FileExists(const char *path)
{
	FILE *fs = fopen(path, "rb");
	if (fs == NULL) {
		return false;
	}
	fclose(fs);
	return true;
}

static void AddDefaultPalette(const char *path)
{
	char *default_ptr = DefaultPointerPath(path);
	char *doom_pal;
	bool pointer_good;

	pointer_good = FileExists(default_ptr);
	free(default_ptr);
	if (pointer_good) {
		return;
	}

	// Pointer not good. We want to point it to the Doom palette file.
	// Is it there?
	doom_pal = StringJoin("/", path, "Doom.png", NULL);
	if (!FileExists(doom_pal)) {
		WriteDoomPalette(doom_pal);
	}
	free(doom_pal);

	SetDefaultPointer(path, "Doom.png");
}

const char *PAL_GetPalettesPath(void)
{
	static char *result = NULL;
	const char *home;

	if (result != NULL) {
		return result;
	}

	home = getenv("HOME");
	assert(home != NULL);

	result = MakeDirectories(home, ".config", "WadGadget",
	                         "Palettes", NULL);
	assert(result != NULL);
	AddDefaultPalette(result);

	return result;
}
