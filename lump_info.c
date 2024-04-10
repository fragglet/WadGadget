
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "wad_file.h"

struct lump_type {
	bool (*check)(struct wad_file_entry *ent, uint8_t *buf);
	void (*format)(struct wad_file_entry *ent, uint8_t *buf,
	               char *descr_buf, size_t descr_buf_len);
};

struct sound_header {
	uint16_t format;
	uint16_t sample_rate;
	uint32_t num_samples;
};

struct patch_header {
	uint16_t width, height;
	int16_t xoff, yoff;
};

struct lump_description {
	const char *name, *description;
};

struct sized_lump {
	int size;
	const char *description;
};

static const struct lump_description special_lumps[] = {
	{"PLAYPAL",   "VGA palette"},
	{"TINTTAB",   "Translucency table"},
	{"XLATAB",    "Translucency table"},
	{"AUTOPAGE",  "Map background texture"},
	{"GENMIDI",   "OPL FM synth instrs."},
	{"DMXGUS",    "GUS instr. mappings"},
	{"DMXGUSC",   "GUS instr. mappings"},
	{"TEXTURE1",  "Texture table"},
	{"TEXTURE2",  "Texture table (reg.)"},
	{"PNAMES",    "Wall patch list"},
	{NULL, NULL},
};

static const struct lump_description level_lumps[] = {
	{"THINGS",    "Level things data"},
	{"LINEDEFS",  "Level linedef data"},
	{"SIDEDEFS",  "Level sidedef data"},
	{"VERTEXES",  "Level vertex data"},
	{"SEGS",      "Level wall segments"},
	{"SSECTORS",  "Level subsectors"},
	{"NODES",     "Level BSP nodes"},
	{"SECTORS",   "Level sector data"},
	{"REJECT",    "Level reject table"},
	{"BLOCKMAP",  "Level blockmap data"},
	{"BEHAVIOR",  "Hexen compiled scripts"},
	{"SCRIPTS",   "Hexen script source"},
	{"LEAFS",     "PSX/D64 node leaves"},
	{"LIGHTS",    "PSX/D64 colored lights"},
	{"MACROS",    "Doom 64 Macros"},
	{"GL_VERT",   "OpenGL extra vertices"},
	{"GL_SEGS",   "OpenGL line segments"},
	{"GL_SSECT",  "OpenGL subsectors"},
	{"GL_NODES",  "OpenGL BSP nodes"},
	{"GL_PVS",    "Potential Vis. Set"},
	{"TEXTMAP",   "UDMF level data"},
	{"DIALOGUE",  "Strife conversations"},
	{"ZNODES",    "UDMF BSP data"},
	{"ENDMAP",    "UDMF end of level"},
	{NULL, NULL},
};

static const struct sized_lump lumps_by_size[] = {
	{8704,   "Light translation map"},
	{64000,  "Fullscreen image"},
	{4000,   "Text mode screen"},
	{4096,   "Floor/ceiling texture"},
	{256,    "Color translation table"},
	{0,      "Empty"},
};

static const char *LookupDescription(const struct lump_description *table,
                                     struct wad_file_entry *ent)
{
	int i;

	for (i = 0; table[i].name != NULL; i++) {
		if (!strncmp(ent->name, table[i].name, 8)) {
			return table[i].description;
		}
	}

	return NULL;
}

// Lump with one of the standard "level" names, eg. "LINEDEFS", "SECTORS", etc.

static bool LevelLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return LookupDescription(level_lumps, ent) != NULL;
}

static void LevelLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                            char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "%s",
	         LookupDescription(level_lumps, ent));
}

const struct lump_type lump_type_level = {
	LevelLumpCheck,
	LevelLumpFormat,
};

// "Special" one-of-a-kind lumps that are listed in the special_lumps array.

static bool SpecialLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return LookupDescription(special_lumps, ent) != NULL;
}

static void SpecialLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                              char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "%s",
	         LookupDescription(special_lumps, ent));
}

const struct lump_type lump_type_special = {
	SpecialLumpCheck,
	SpecialLumpFormat,
};

// PCM sound effects.

static bool SoundLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	struct sound_header *sound = (struct sound_header *) buf;

	return ent->size >= 16 && sound->format == 3
	   && (sound->sample_rate == 8000 || sound->sample_rate == 11025
	    || sound->sample_rate == 22050 || sound->sample_rate == 44100);
}

static void SoundLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                              char *descr_buf, size_t descr_buf_len)
{
	struct sound_header *sound = (struct sound_header *) buf;

	snprintf(descr_buf, descr_buf_len,
	         "PCM sound effect\nSample rate: %d hz\nLength: %0.02fs",
	         sound->sample_rate,
	         (float) sound->num_samples / sound->sample_rate);
}

const struct lump_type lump_type_sound = {
	SoundLumpCheck,
	SoundLumpFormat,
};

// Graphic lump (sprite, texture, etc.)

static bool GraphicLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	struct patch_header *patch = (struct patch_header *) buf;

	return ent->size >= 8 && patch->width > 0 && patch->height > 0
	    && patch->width <= 320 && patch->height <= 200
	    && patch->xoff >= -256 && patch->xoff < 256
	    && patch->yoff >= -256 && patch->yoff < 256;
}

static void GraphicLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                              char *descr_buf, size_t descr_buf_len)
{
	struct patch_header *patch = (struct patch_header *) buf;

	snprintf(descr_buf, descr_buf_len,
	         "Graphic\nDimensions: %dx%d\nOffsets: %d, %d",
	         patch->width, patch->height,
	         patch->xoff, patch->yoff);
}

const struct lump_type lump_type_graphic = {
	GraphicLumpCheck,
	GraphicLumpFormat,
};

static bool MusicLumpCheck(struct wad_file_entry *ent, uint8_t *buf)
{
	return ent->size >= 4
	    && (!memcmp(buf, "MThd", 4) || !memcmp(buf, "MUS\x1a", 4));
}

static void MusicLumpFormat(struct wad_file_entry *ent, uint8_t *buf,
                            char *descr_buf, size_t descr_buf_len)
{
	snprintf(descr_buf, descr_buf_len, "%s music track",
	         !memcmp(buf, "MThd", 4) ? "MIDI" : "DMX MUS");
}

const struct lump_type lump_type_music = {
	MusicLumpCheck,
	MusicLumpFormat,
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

	ep = buf[2]; map = buf[3];
	if (ep != 1) {
		snprintf(level_buf, sizeof(level_buf), "E%dM%d", ep, map);
	} else if (map < 10) {
		snprintf(level_buf, sizeof(level_buf),
		         "E1M%d or MAP%02d", map, map);
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

const struct lump_type lump_type_demo = {
	DemoLumpCheck,
	DemoLumpFormat,
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

	snprintf(descr_buf, descr_buf_len,
	         "PC speaker sound\nLength: %0.02fs", (float) len / 140);
}

const struct lump_type lump_type_pcspeaker = {
	PcSpeakerLumpCheck,
	PcSpeakerLumpFormat,
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

const struct lump_type lump_type_sized = {
	SizedLumpCheck,
	SizedLumpFormat,
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
	         "%02x %02x %02x %02x %02x %02x %02x %02x",
	         buf[0], buf[1], buf[2], buf[3],
	         buf[4], buf[5], buf[6], buf[7]);
}

const struct lump_type lump_type_unknown = {
	UnknownLumpCheck,
	UnknownLumpFormat,
};

static const struct lump_type *lump_types[] = {
	&lump_type_level,
	&lump_type_special,
	&lump_type_sound,
	&lump_type_graphic,
	&lump_type_music,
	&lump_type_demo,
	&lump_type_pcspeaker,
	&lump_type_sized,
	&lump_type_unknown,
};

const struct lump_type *LI_IdentifyLump(struct wad_file *f,
                                        unsigned int lump_index)
{
	struct wad_file_entry *ent;
	uint8_t buf[8];
	int i;

	ent = &W_GetDirectory(f)[lump_index];

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
