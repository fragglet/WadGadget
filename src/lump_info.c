//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "common.h"
#include "conv/audio.h"
#include "conv/graphic.h"
#include "fs/wad_file.h"
#include "stringlib.h"

struct wad_file;

struct lump_section {
	const char *start1, *start2;
	const char *end1, *end2;
};

struct lump_type {
	bool (*check)(struct wad_file_entry *ent, uint8_t *buf);
	void (*format)(struct wad_file_entry *ent, uint8_t *buf,
	               char *descr_buf, size_t descr_buf_len);
	const char *(*help_page)(struct wad_file_entry *ent);
	const char *extension;
};

struct lump_description {
	const char *name, *description;
	const char *help_page;
};

struct sized_lump {
	int size;
	const char *description;
	const char *help_page;
};

const struct lump_section lump_section_sprites = {
    "S_START",
    "SS_START",
    "S_END",
    "SS_END",
};

const struct lump_section lump_section_patches = {
    "P_START",
    "PP_START",
    "P_END",
    "PP_END",
};

const struct lump_section lump_section_flats = {
    "F_START",
    "FF_START",
    "F_END",
    "FF_END",
};

const struct lump_section lump_section_colormaps = {
    "C_START",
    "C_START",
    "C_END",
    "C_END",
};

static const struct lump_description special_lumps[] = {
    {"TINTTAB",  "Translucency table",      NULL                        },
    {"XLATAB",   "Translucency table",      NULL                        },
    {"AUTOPAGE", "Map background texture",  NULL                        },
    {"GENMIDI",  "OPL FM synth instrs.",    NULL                        },
    {"DEHACKED", "Dehacked patch",          "mbfedit.md#6-dehacked-lump"},
    {"OPTIONS",  "Boom config overrides",   "mbfedit.md#7-options-lump" },
    {"ANIMATED", "Animated texture defs",   "boomref.md#animated-format"},
    {"SWITCHES", "Switch texture defs",     "boomref.md#switches-format"},
    {"SINETABL", "sin() table",             NULL                        },
    {"TANGTABL", "tan() table",             NULL                        },
    {"TANTOANG", "arctan() table",          NULL                        },
    {"GAMMATBL", "Gamma correction tables", NULL                        },
    {NULL,       NULL,                      NULL                        },
};

static const struct lump_description level_lumps[] = {
    {"THINGS",   "Level things data",      "uds.md#4-2-things"   },
    {"LINEDEFS", "Level linedef data",     "uds.md#4-3-linedefs" },
    {"SIDEDEFS", "Level sidedef data",     "uds.md#4-4-sidedefs" },
    {"VERTEXES", "Level vertex data",      "uds.md#4-5-vertexes" },
    {"SEGS",     "Level wall segments",    "uds.md#4-6-segs"     },
    {"SSECTORS", "Level subsectors",       "uds.md#4-7-ssectors" },
    {"NODES",    "Level BSP nodes",        "uds.md#4-8-nodes"    },
    {"SECTORS",  "Level sector data",      "uds.md#4-9-sectors"  },
    {"REJECT",   "Level reject table",     "uds.md#4-10-reject"  },
    {"BLOCKMAP", "Level blockmap data",    "uds.md#4-11-blockmap"},
    {"BEHAVIOR", "Hexen compiled scripts",
     "hexen_specs.md#4-hexen-script-language"                    },
    {"SCRIPTS",  "Hexen script source",    NULL                  },
    {"LEAFS",    "PSX/D64 node leaves",    NULL                  },
    {"LIGHTS",   "PSX/D64 colored lights", NULL                  },
    {"MACROS",   "Doom 64 Macros",         NULL                  },
    {"GL_VERT",  "OpenGL extra vertices",  NULL                  },
    {"GL_SEGS",  "OpenGL line segments",   NULL                  },
    {"GL_SSECT", "OpenGL subsectors",      NULL                  },
    {"GL_NODES", "OpenGL BSP nodes",       NULL                  },
    {"GL_PVS",   "Potential Vis. Set",     NULL                  },
    {"TEXTMAP",  "UDMF level data",        NULL                  },
    {"DIALOGUE", "Strife conversations",   NULL                  },
    {"ZNODES",   "UDMF BSP data",          NULL                  },
    {"ENDMAP",   "UDMF end of level",      NULL                  },
    {NULL,       NULL,                     NULL                  },
};

// TODO: Help pages for these:
static const struct sized_lump lumps_by_size[] = {
    {4000, "Text mode screen",        "uds.md#8-3-endoom"                  },
    {256,  "Color translation table", "boomref.md#color-translation-tables"},
};

bool LI_LumpInSection(struct wad_file *wf, unsigned int lump_index,
                      const struct lump_section *section)
{
	const struct wad_file_entry *dir = W_GetDirectory(wf);
	int num_lumps = W_NumLumps(wf);
	int i;

	for (i = lump_index; i >= 0 && i < num_lumps; i--) {
		if (!strncasecmp(dir[i].name, section->start1, 8) ||
		    !strncasecmp(dir[i].name, section->start2, 8)) {
			break;
		}
	}
	if (i < 0) {
		return false;
	}
	for (i = lump_index + 1; i < num_lumps; i++) {
		if (!strncasecmp(dir[i].name, section->end1, 8) ||
		    !strncasecmp(dir[i].name, section->end2, 8)) {
			return true;
		}
	}
	return false;
}

static const struct lump_description *
LookupByName(const struct lump_description *table, const char *name)
{
	int i;

	for (i = 0; table[i].name != NULL; i++) {
		if (!strncmp(name, table[i].name, 8)) {
			return &table[i];
		}
	}

	return NULL;
}

static bool EmptyLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size == 0;
}

static void EmptyLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                            char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "Empty");
}

const struct lump_type lump_type_empty = {
    EmptyLumpCheck,
    EmptyLumpFormat,
    NULL,
    ".txt",
};

// Lump with one of the standard "level" names, eg. "LINEDEFS", "SECTORS",
// etc.

static bool LevelLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return LookupByName(level_lumps, ent->name) != NULL;
}

static void LevelLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                            char *descr_buf, size_t descr_buf_len)
{
	const struct lump_description *ld =
	    LookupByName(level_lumps, ent->name);
	snprintf(descr_buf, descr_buf_len, "%s", ld->description);
}

static const char *LevelLumpHelpPage(struct wad_file_entry *ent)
{
	return LookupByName(level_lumps, ent->name)->help_page;
}

const struct lump_type lump_type_level = {
    LevelLumpCheck,
    LevelLumpFormat,
    LevelLumpHelpPage,
};

// "Special" one-of-a-kind lumps that are listed in the special_lumps array.

static bool SpecialLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return LookupByName(special_lumps, ent->name) != NULL;
}

static void SpecialLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                              char *descr_buf, size_t descr_buf_len)
{
	const struct lump_description *ld =
	    LookupByName(special_lumps, ent->name);
	snprintf(descr_buf, descr_buf_len, "%s", ld->description);
}

static const char *SpecialLumpHelpPage(struct wad_file_entry *ent)
{
	return LookupByName(special_lumps, ent->name)->help_page;
}

const struct lump_type lump_type_special = {
    SpecialLumpCheck,
    SpecialLumpFormat,
    SpecialLumpHelpPage,
};

// PCM sound effects.

static bool SoundLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	struct sound_header sound = *((struct sound_header *) buf);
	S_SwapSoundHeader(&sound);
	return ent->size >= 16 && sound.format == 3 &&
	       (sound.sample_rate == 8000 || sound.sample_rate == 11025 ||
	        sound.sample_rate == 22050 || sound.sample_rate == 44100);
}

static void SoundLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                            char *descr_buf, size_t descr_buf_len)
{
	struct sound_header sound = *((struct sound_header *) buf);
	S_SwapSoundHeader(&sound);

	snprintf(descr_buf, descr_buf_len,
	         "PCM sound effect\nSample rate: %d hz\nLength: %0.02fs",
	         sound.sample_rate,
	         (float) sound.num_samples / sound.sample_rate);
}

static const char *SoundLumpHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#7-2-soundcard-sound-effects";
}

const struct lump_type lump_type_sound = {
    SoundLumpCheck,
    SoundLumpFormat,
    SoundLumpHelpPage,
    ".wav",
};

// Creative Labs .voc sound effects, used in Duke3D/ROTT .rts files (rather
// than Doom's native sound effect format).

static bool VocSoundCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	// Note that this does mean if there's a plain text lump that starts
	// with the word "Creative" then it will be misidentified, but it
	// seems pretty unlikely.
	return ent->size > 16 && !memcmp(buf, "Creative", 8);
}

static void VocSoundFormat(struct wad_file_entry *ent, uint8_t *buf,
                           char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "Creative VOC sound effect");
}

const struct lump_type lump_type_sound_voc = {
    VocSoundCheck,
    VocSoundFormat,
    NULL,
    ".voc",
};

// Graphic lump (sprite, texture, etc.)

static bool GraphicLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	struct patch_header patch = *((struct patch_header *) buf);
	V_SwapPatchHeader(&patch);

	return ent->size >= 8 && patch.width > 0 && patch.height > 0 &&
	       patch.width <= 320 && patch.height <= 200 &&
	       patch.leftoffset >= -256 && patch.leftoffset < 256 &&
	       patch.topoffset >= -256 && patch.topoffset < 256;
}

static void GraphicLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                              char *descr_buf, size_t descr_buf_len)
{
	struct patch_header patch = *((struct patch_header *) buf);
	V_SwapPatchHeader(&patch);

	snprintf(descr_buf, descr_buf_len,
	         "Graphic\nDimensions: %dx%d\nOffsets: %d, %d", patch.width,
	         patch.height, patch.leftoffset, patch.topoffset);
}

static const char *GraphicLumpHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#5-1-picture-format";
}

const struct lump_type lump_type_graphic = {
    GraphicLumpCheck,
    GraphicLumpFormat,
    GraphicLumpHelpPage,
    ".png",
};

// Floor/ceiling texture.

static bool FlatLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	// Flats are a special case that we identify by looking at lump size
	// and WAD section.
	assert(false);
}

static void FlatLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                           char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "Floor/ceiling texture");
}

static const char *FlatLumpHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#chapter-6-flats-floor-and-ceiling-textures";
}

const struct lump_type lump_type_flat = {
    FlatLumpCheck,
    FlatLumpFormat,
    FlatLumpHelpPage,
    ".flat.png",
};

// DMX .MUS format.

static bool MusLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size >= 4 && !memcmp(buf, "MUS\x1a", 4);
}

static void MusLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                          char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "DMX MUS music track");
}

const struct lump_type lump_type_mus = {
    MusLumpCheck,
    MusLumpFormat,
    NULL,
    ".mid",
};

// Embedded MIDI file (also supported by DMX)

static bool MidiLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size >= 4 && !memcmp(buf, "MThd", 4);
}

static void MidiLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                           char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "MIDI music track");
}

const struct lump_type lump_type_midi = {
    MidiLumpCheck,
    MidiLumpFormat,
    NULL,
    ".mid",
};

// DMX GUS instrument mappings.
static bool DmxGusCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return StringHasPrefix(ent->name, "DMXGUS");
}

static void DmxGusFormat(struct wad_file_entry *ent, uint8_t *buf,
                         char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "GUS instrument mappings");
}

static const char *DmxGusHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#7-5-dmxgus";
}

const struct lump_type lump_type_dmxgus = {
    DmxGusCheck,
    DmxGusFormat,
    DmxGusHelpPage,
    ".ini",
};

static const struct {
	int code;
	const char *str;
} versions[] = {
    {106, "v1.666"},
    {107, "v1.7/1.7a"},
    {108, "v1.8"},
    {109, "v1.9"},
    {0},
};

static const char *VersionCodeString(int code)
{
	int i;

	if (code < 5) {
		return "v1.2";
	}

	for (i = 0; versions[i].code != 0; i++) {
		if (code == versions[i].code) {
			return versions[i].str;
		}
	}

	return "v?.?";
}

static bool DemoLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return !strncasecmp(ent->name, "DEMO", 4);
}

static void DemoLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                           char *descr_buf, size_t descr_buf_len)
{
	char level_buf[20];
	const char *modestr;
	int ep, map;

	ep = buf[2];
	map = buf[3];
	if (ep != 1) {
		snprintf(level_buf, sizeof(level_buf), "E%dM%d", ep, map);
	} else if (map < 10) {
		snprintf(level_buf, sizeof(level_buf), "E1M%d or MAP%02d", map,
		         map);
	} else {
		snprintf(level_buf, sizeof(level_buf), "MAP%02d", map);
	}

	switch (buf[4]) {
	case 0:
		modestr = "SP/Coop";
		break;
	case 1:
		modestr = "Deathmatch";
		break;
	case 2:
		modestr = "Altdeath";
		break;
	default:
		modestr = "";
		break;
	}

	snprintf(descr_buf, descr_buf_len,
	         "Demo recording\nVersion: %s, Skill: %d\n%s; %s",
	         VersionCodeString(buf[0]), buf[1], modestr, level_buf);
}

static const char *DemoLumpHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#8-6-demos";
}

const struct lump_type lump_type_demo = {
    DemoLumpCheck,
    DemoLumpFormat,
    DemoLumpHelpPage,
};

static bool PcSpeakerLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	size_t len;

	if (strncasecmp(ent->name, "DP", 2) != 0) {
		return false;
	}

	len = buf[2] | (buf[3] << 8);
	return buf[0] == 0 && buf[1] == 0 && len + 4 == ent->size;
}

static void PcSpeakerLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                                char *descr_buf, size_t descr_buf_len)
{
	size_t len = buf[2] | (buf[3] << 8);

	snprintf(descr_buf, descr_buf_len, "PC speaker sound\nLength: %0.02fs",
	         (float) len / 140);
}

const struct lump_type lump_type_pcspeaker = {
    PcSpeakerLumpCheck,
    PcSpeakerLumpFormat,
};

static bool DehackedLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return !strncasecmp(ent->name, "DEHACKED", 8);
}

static void DehackedLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                               char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "Dehacked patch");
}

const struct lump_type lump_type_dehacked = {
    DehackedLumpCheck,
    DehackedLumpFormat,
    NULL,
    ".deh",
};

static bool PaletteCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return (!strcasecmp(ent->name, "PLAYPAL") ||
	        !strcasecmp(ent->name, "PALPREF")) &&
	       ent->size > 0 && (ent->size % (256 * 3)) == 0;
}

static void PaletteFormat(struct wad_file_entry *ent, uint8_t *buf,
                          char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "VGA palette (%d palettes)",
	         ent->size / (256 * 3));
}

static const char *PaletteHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#8-1-playpal";
}

const struct lump_type lump_type_palette = {
    PaletteCheck,
    PaletteFormat,
    PaletteHelpPage,
    ".png",
};

static bool ColormapCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size > 0 && (ent->size % 256) == 0 &&
	       (!strncasecmp(ent->name, "COLORMAP", 8) ||
	        !strncasecmp(ent->name, "FOGMAP", 8));
}

static void ColormapFormat(struct wad_file_entry *ent, uint8_t *buf,
                           char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "Colormap (%d maps)",
	         ent->size / 256);
}

static const char *ColormapHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#8-2-colormap";
}

const struct lump_type lump_type_colormap = {
    ColormapCheck,
    ColormapFormat,
    ColormapHelpPage,
    ".cmap.png",
};

// Lump types identified by fixed size. This type is last in the identification
// list before "unknown" because it uses the least information to make the
// identification.

static const struct sized_lump *LumpTypeForSize(unsigned int len)
{
	int i;

	for (i = 0; i < arrlen(lumps_by_size); i++) {
		if (lumps_by_size[i].size == len) {
			return &lumps_by_size[i];
		}
	}

	return NULL;
}

static bool SizedLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return LumpTypeForSize(ent->size) != NULL;
}

static void SizedLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                            char *descr_buf, size_t descr_buf_len)
{
	const struct sized_lump *lt = LumpTypeForSize(ent->size);
	snprintf(descr_buf, descr_buf_len, "%s", lt->description);
}

static const char *SizedLumpHelpPage(struct wad_file_entry *ent)
{
	return LumpTypeForSize(ent->size)->help_page;
}

const struct lump_type lump_type_sized = {
    SizedLumpCheck,
    SizedLumpFormat,
    SizedLumpHelpPage,
};

// Plain text lumps, as found in Hexen IWAD and also used by source ports.

static bool IsPlainText(const unsigned char *buf, size_t buf_len)
{
	static const char whitespace_chars[] = "\t\n\r";
	int i;

	for (i = 0; i < buf_len; i++) {
		unsigned char c = buf[i];
		if (c >= 0x7f || c == 0 ||
		    (c < 0x20 && strchr(whitespace_chars, c) == NULL)) {
			return false;
		}
	}

	return true;
}

static bool PlainTextLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size >= LUMP_HEADER_LEN &&
	       IsPlainText(buf, LUMP_HEADER_LEN);
}

static void PlainTextLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                                char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "%s", "Plain text");
}

const struct lump_type lump_type_plaintext = {
    PlainTextLumpCheck,
    PlainTextLumpFormat,
    NULL,
    ".txt",
};

// TEXTURE1 etc.
static bool TexturesCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return StringHasPrefix(ent->name, "TEXTURE") && ent->size >= 4;
}

static void TexturesFormat(struct wad_file_entry *ent, uint8_t *buf,
                           char *descr_buf, size_t descr_buf_len)
{
	uint32_t cnt;
	memcpy(&cnt, buf, sizeof(uint32_t));
	SwapLE32(&cnt);
	snprintf(descr_buf, descr_buf_len, "Texture directory%s\n%d textures",
	         !strncmp(ent->name, "TEXTURE2", 8) ? " (reg.)" : "", cnt);
}

static const char *TexturesHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#8-4-texture1-and-texture2";
}

const struct lump_type lump_type_textures = {
    TexturesCheck,
    TexturesFormat,
    TexturesHelpPage,
    ".txt",
};

// Patch names list lump
static bool PnamesCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return !strcmp(ent->name, "PNAMES") && ent->size >= 4;
}

static void PnamesFormat(struct wad_file_entry *ent, uint8_t *buf,
                         char *descr_buf, size_t descr_buf_len)
{
	uint32_t cnt;
	memcpy(&cnt, buf, sizeof(uint32_t));
	SwapLE32(&cnt);
	snprintf(descr_buf, descr_buf_len, "Wall patch list\n%d patch names",
	         cnt);
}

static const char *PnamesHelpPage(struct wad_file_entry *ent)
{
	return "uds.md#8-5-pnames";
}

const struct lump_type lump_type_pnames = {
    PnamesCheck,
    PnamesFormat,
    PnamesHelpPage,
    ".txt",
};

// Hexen fullscreen image
static bool FullscreenImageCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size == 64000;
}

static void FullscreenImageFormat(struct wad_file_entry *ent, uint8_t *buf,
                                  char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len,
	         "Hexen fullscreen image\nDimensions: 320x200");
}

const struct lump_type lump_type_fullscreen_image = {
    FullscreenImageCheck,
    FullscreenImageFormat,
    NULL,
    ".fullscreen.png",
};

// Hexen hires loading screen
static bool HexenHiresImageCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return !strcasecmp(ent->name, "STARTUP");
}

static void HexenHiresImageFormat(struct wad_file_entry *ent, uint8_t *buf,
                                  char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len,
	         "Hexen startup screen\nDimensions: 640x480");
}

const struct lump_type lump_type_hexen_hires_image = {
    HexenHiresImageCheck,
    HexenHiresImageFormat,
    NULL,
    ".hires.png",
};

// Fallback, "generic lump"

static bool UnknownLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return true;
}

static void UnknownLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                              char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len,
	         "%02x %02x %02x %02x %02x %02x %02x %02x", buf[0], buf[1],
	         buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
}

const struct lump_type lump_type_unknown = {
    UnknownLumpCheck,
    UnknownLumpFormat,
};

static const struct lump_type *lump_types[] = {
    &lump_type_empty,
    &lump_type_dehacked,
    &lump_type_level,
    &lump_type_special,
    &lump_type_sound,
    &lump_type_sound_voc,
    &lump_type_textures,
    &lump_type_pnames,
    &lump_type_graphic,
    &lump_type_mus,
    &lump_type_dmxgus,
    &lump_type_midi,
    &lump_type_demo,
    &lump_type_pcspeaker,
    &lump_type_fullscreen_image,
    &lump_type_hexen_hires_image,
    &lump_type_palette,
    &lump_type_colormap,
    &lump_type_sized,
    &lump_type_plaintext,
    &lump_type_unknown,
};

const struct lump_type *LI_IdentifyLump(struct wad_file *f,
                                        unsigned int lump_index)
{
	struct wad_file_entry *ent;
	uint8_t buf[8];
	int i;

	ent = &W_GetDirectory(f)[lump_index];

	// Flats are a special case where we look at lump size but also
	// check the section of the WAD; it must be between
	// F_START/F_END markers.
	if (ent->size >= 4096 && (ent->size % 64) == 0 &&
	    LI_LumpInSection(f, lump_index, &lump_section_flats)) {
		return &lump_type_flat;
	}

	if (ent->size > 0 && (ent->size % 256) == 0 &&
	    LI_LumpInSection(f, lump_index, &lump_section_colormaps)) {
		return &lump_type_colormap;
	}

	memset(buf, 0, sizeof(buf));
	W_ReadLumpHeader(f, lump_index, buf, sizeof(buf));

	for (i = 0; i < arrlen(lump_types); i++) {
		if (lump_types[i]->check(ent, buf)) {
			return lump_types[i];
		}
	}

	return NULL;
}

const char *LI_DescribeLump(const struct lump_type *t, struct wad_file *f,
                            unsigned int lump_index)
{
	static char description_buf[128];
	struct wad_file_entry *ent;
	uint8_t buf[8];

	ent = &W_GetDirectory(f)[lump_index];

	memset(buf, 0, sizeof(buf));
	W_ReadLumpHeader(f, lump_index, buf, sizeof(buf));

	t->format(ent, buf, description_buf, sizeof(description_buf));

	return description_buf;
}

const char *LI_GetHelpPage(const struct lump_type *t, struct wad_file *f,
                           unsigned int lump_index)
{
	struct wad_file_entry *ent;

	if (t->help_page == NULL) {
		return NULL;
	}

	ent = &W_GetDirectory(f)[lump_index];
	return t->help_page(ent);
}

const char *LI_GetExtension(const struct lump_type *lt, bool convert)
{
	if (convert && lt->extension != NULL) {
		return lt->extension;
	}

	// A special case.
	if (lt == &lump_type_mus) {
		return ".mus";
	}

	return ".lmp";
}
