
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "wad_file.h"

static char description_buf[128];

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

static const struct lump_description special_lumps[] = {
	{"PLAYPAL",   "VGA palette"},
	{"COLORMAP",  "Light translation map"},
	{"TINTTAB",   "Translucency table"},
	{"XLATAB",    "Translucency table"},
	{"AUTOPAGE",  "Map background texture"},
	{"ENDOOM",    "Text mode exit screen"},
	{"ENDTEXT",   "Text mode exit screen"},
	{"ENDSTRF",   "Text mode exit screen"},
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

static const char *CheckForEmptyLump(struct wad_file_entry *ent,
                                     void *buf)
{
	if (ent->size == 0) {
		return "Empty";
	}
	return NULL;
}

static const char *CheckForLevelLump(struct wad_file_entry *ent, void *buf)
{
	return LookupDescription(level_lumps, ent);
}

static const char *CheckForSpecialLump(struct wad_file_entry *ent, void *buf)
{
	return LookupDescription(special_lumps, ent);
}

static const char *CheckForSoundLump(struct wad_file_entry *ent,
                                     void *buf)
{
	struct sound_header *sound = buf;

	if (ent->size >= 16 && sound->format == 3
	 && (sound->sample_rate == 8000 || sound->sample_rate == 11025
	  || sound->sample_rate == 22050 || sound->sample_rate == 44100)) {
		snprintf(description_buf, sizeof(description_buf),
		         "Sound, %d hz\nLength: %0.02fs",
		         sound->sample_rate,
		         (float) sound->num_samples / sound->sample_rate);
		return description_buf;
	}
	return NULL;
}

static const char *CheckForGraphicLump(struct wad_file_entry *ent,
                                       void *buf)
{
	struct patch_header *patch = buf;

	if (ent->size >= 8 && patch->width > 0 && patch->height > 0
	 && patch->width <= 320 && patch->height <= 200
	 && patch->xoff > -192 && patch->xoff <= 192
	 && patch->yoff > -192 && patch->yoff <= 192) {
		snprintf(description_buf, sizeof(description_buf),
		         "Graphic (%dx%d)\nOffsets: %d, %d",
		         patch->width, patch->height,
		         patch->xoff, patch->yoff);
		return description_buf;
	}
	return NULL;
}

static const char *CheckForMusicLump(struct wad_file_entry *ent,
                                     void *buf)
{
	if (ent->size < 4) {
		return NULL;
	}

	if (!memcmp(buf, "MThd", 4)) {
		return "MIDI music track";
	}

	if (!memcmp(buf, "MUS\x1a", 4)) {
		return "DMX MUS music track";
	}

	return NULL;
}

static const char *CheckForFlat(struct wad_file_entry *ent, void *buf)
{
	if (ent->size == 4096) {
		return "Floor/ceiling texture";
	}
	return NULL;
}

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

static const char *CheckForDemo(struct wad_file_entry *ent, void *buf)
{
	char level_buf[20];
	const char *modestr;
	uint8_t *bytes = buf;
	int ep, map;

	if (strncasecmp(ent->name, "DEMO", 4) != 0) {
		return NULL;
	}

	ep = bytes[2]; map = bytes[3];
	if (ep != 1) {
		snprintf(level_buf, sizeof(level_buf), "E%dM%d", ep, map);
	} else if (map < 10) {
		snprintf(level_buf, sizeof(level_buf),
		         "E1M%d or MAP%02d", map, map);
	} else {
		snprintf(level_buf, sizeof(level_buf), "MAP%02d", map);
	}

	switch (bytes[4]) {
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

	snprintf(description_buf, sizeof(description_buf),
	         "Demo: %s, skill %d\n%s; %s",
	         VersionCodeString(bytes[0]), bytes[1], modestr, level_buf);
	return description_buf;
}

static const char *CheckForPcSpeaker(struct wad_file_entry *ent, void *buf)
{
	uint8_t *bytes = buf;
	size_t len;

	if (strncasecmp(ent->name, "DP", 2) != 0) {
		return NULL;
	}

	len = bytes[2] | (bytes[3] << 8);
	if (bytes[0] == 0 && bytes[1] == 0 && len + 4 == ent->size) {
		snprintf(description_buf, sizeof(description_buf),
		         "PC speaker sound, %0.02fs", (float) len / 140);
		return description_buf;
	}

	return NULL;
}

typedef const char *(*check_function)(struct wad_file_entry *ent, void *buf);

static check_function check_functions[] = {
	CheckForEmptyLump,
	CheckForLevelLump,
	CheckForSpecialLump,
	CheckForPcSpeaker,
	CheckForSoundLump,
	CheckForGraphicLump,
	CheckForMusicLump,
	CheckForFlat,
	CheckForDemo,
	NULL,
};

const char *GetLumpDescription(struct wad_file *f,
                               unsigned int lump_index)
{
	struct wad_file_entry *ent;
	uint8_t buf[8];
	int i;

	ent = &W_GetDirectory(f)[lump_index];

	memset(buf, 0, sizeof(buf));
	W_ReadLumpHeader(f, lump_index, buf, sizeof(buf));

	for (i = 0; check_functions[i] != NULL; i++) {
		const char *result = check_functions[i](ent, buf);
		if (result != NULL) {
			return result;
		}
	}

	snprintf(description_buf, sizeof(description_buf),
	         "%02x %02x %02x %02x %02x %02x %02x %02x",
	         buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
	return description_buf;
}

