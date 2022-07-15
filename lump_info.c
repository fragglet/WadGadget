
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

static const char *CheckForEmptyLump(struct wad_file_entry *ent,
                                     void *buf)
{
	if (ent->size == 0) {
		return "Empty";
	}
	return NULL;
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

typedef const char *(*check_function)(struct wad_file_entry *ent, void *buf);

static check_function check_functions[] = {
	CheckForEmptyLump,
	CheckForSoundLump,
	CheckForGraphicLump,
	CheckForMusicLump,
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

