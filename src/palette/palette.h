//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

struct palette_entry {
	uint8_t r, g, b;
};

struct palette {
	struct palette_entry entries[256];
};

// A collection of palettes - ie. the decoded contents of a PLAYPAL lump.
struct palette_set {
	struct palette *palettes;
	size_t num_palettes;
};

extern const struct palette doom_palette;

struct palette_set *PAL_FromImageFile(VFILE *input);
VFILE *PAL_ToImageFile(struct palette_set *set);
VFILE *PAL_MarshalPaletteSet(const struct palette_set *set);
struct palette_set *PAL_UnmarshalPaletteSet(VFILE *input);
void PAL_FreePaletteSet(struct palette_set *set);
const char *PAL_GetPalettesPath(void);
