//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "fs/wad_file.h"

struct lump_type;
struct lump_section;

extern const struct lump_type lump_type_level;
extern const struct lump_type lump_type_special;
extern const struct lump_type lump_type_sound;
extern const struct lump_type lump_type_flat;
extern const struct lump_type lump_type_graphic;
extern const struct lump_type lump_type_mus;
extern const struct lump_type lump_type_midi;
extern const struct lump_type lump_type_demo;
extern const struct lump_type lump_type_pcspeaker;
extern const struct lump_type lump_type_dehacked;
extern const struct lump_type lump_type_sized;
extern const struct lump_type lump_type_plaintext;
extern const struct lump_type lump_type_fullscreen_image;
extern const struct lump_type lump_type_hexen_hires_image;
extern const struct lump_type lump_type_pnames;
extern const struct lump_type lump_type_textures;
extern const struct lump_type lump_type_unknown;

extern const struct lump_section lump_section_sprites;
extern const struct lump_section lump_section_patches;
extern const struct lump_section lump_section_flats;

const struct lump_type *LI_IdentifyLump(struct wad_file *f,
                                        unsigned int lump_index);
const char *LI_DescribeLump(const struct lump_type *t, struct wad_file *f,
                            unsigned int lump_index);
const char *LI_GetExtension(const struct lump_type *lt, bool convert);
bool LI_LumpInSection(struct wad_file *wf, unsigned int lump_index,
                      const struct lump_section *section);

