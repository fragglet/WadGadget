//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <sndfile.h>
#include <assert.h>

#include "conv/audio.h"
#include "conv/error.h"
#include "common.h"

void S_SwapSoundHeader(struct sound_header *hdr)
{
	SwapLE16(&hdr->format);
	SwapLE16(&hdr->sample_rate);
	SwapLE32(&hdr->num_samples);
};

static sf_count_t SoundFileGetLen(void *user_data)
{
	long pos = vftell(user_data);
	sf_count_t result;

	vfseek(user_data, 0, SEEK_END);
	result = vftell(user_data);
	vfseek(user_data, pos, SEEK_SET);

	return result;
}

static sf_count_t SoundFileSeek(sf_count_t offset, int whence, void *user_data)
{
	return vfseek(user_data, offset, whence);
}

static sf_count_t SoundFileRead(void *ptr, sf_count_t count, void *user_data)
{
	return vfread(ptr, 1, count, user_data);
}

static sf_count_t SoundFileWrite(const void *ptr, sf_count_t count,
                                 void *user_data)
{
	return vfwrite(ptr, 1, count, user_data);
}

static sf_count_t SoundFileTell(void *user_data)
{
	return vftell(user_data);
}

static struct SF_VIRTUAL_IO virt_ops = {
	SoundFileGetLen,
	SoundFileSeek,
	SoundFileRead,
	SoundFileWrite,
	SoundFileTell,
};

static bool ReadNextSample(SF_INFO *sf_info, SNDFILE *sndfile, short *buf,
                           uint8_t *result)
{
	int accum = 0;
	int i;

	if (sf_readf_short(sndfile, buf, 1) != 1) {
		ConversionError("%s", sf_strerror(sndfile));
		return false;
	}

	for (i = 0; i < sf_info->channels; i++) {
		accum += buf[i];
	}

	accum /= sf_info->channels;

	// Take top byte and convert to unsigned.
	return (accum / 256) + 128;
}

VFILE *S_FromAudioFile(VFILE *input)
{
	short *framebuf;
	SF_INFO sf_info;
	SNDFILE *virt;
	VFILE *result;
	struct sound_header hdr;
	uint8_t buf[128];
	size_t nsamples, buf_len;
	bool success = true;

	virt = sf_open_virtual(&virt_ops, SFM_READ, &sf_info, input);
	if (virt == NULL) {
		ConversionError("%s", sf_strerror(NULL));
		vfclose(input);
		return NULL;
	}

	result = vfopenmem(NULL, 0);
	hdr.format = 3;
	hdr.sample_rate = sf_info.samplerate;
	hdr.num_samples = sf_info.frames;
	S_SwapSoundHeader(&hdr);
	vfwrite(&hdr, sizeof(hdr), 1, result);

	framebuf = checked_calloc(sf_info.channels, sizeof(short));
	nsamples = 0;
	buf_len = 0;
	while (nsamples < sf_info.frames) {
		success = ReadNextSample(&sf_info, virt, framebuf,
		                         &buf[buf_len]);
		if (!success) {
			ConversionError("unexpected end of file");
			break;
		}
		++nsamples;
		++buf_len;
		// Is the buffer full? Write it out.
		if (buf_len >= sizeof(buf) || nsamples >= sf_info.frames) {
			assert(vfwrite(buf, 1, buf_len, result) == buf_len);
			buf_len = 0;
		}
	}

	sf_write_sync(virt);
	vfseek(result, 0, SEEK_SET);
	sf_close(virt);

	if (!success) {
		vfclose(result);
		result = NULL;
	}

	free(framebuf);
	vfclose(input);

	return result;
}

VFILE *S_ToAudioFile(VFILE *input)
{
	struct sound_header hdr;
	SNDFILE *out;
	SF_INFO sf_info;
	VFILE *result;
	bool success = true;
	int i;

	if (vfread(&hdr, sizeof(hdr), 1, input) != 1) {
		ConversionError("failed to read sound lump header");
		vfclose(input);
		return NULL;
	}
	S_SwapSoundHeader(&hdr);
	if (hdr.format != 3) {
		ConversionError("sound header has unknown format type (%d)",
		                hdr.format);
		vfclose(input);
		return NULL;
	}

	result = vfopenmem(NULL, 0);
	sf_info.frames = hdr.num_samples;
	sf_info.samplerate = hdr.sample_rate;
	sf_info.channels = 1;
	sf_info.format = SF_FORMAT_WAV|SF_FORMAT_PCM_U8;

	out = sf_open_virtual(&virt_ops, SFM_WRITE, &sf_info, result);
	if (out == NULL) {
		fprintf(stderr, "libsndfile error: %s\n", sf_strerror(NULL));
		assert(out != NULL);
	}

	for (i = 0; i < hdr.num_samples; i++) {
		uint8_t sample;
		short sample16;
		success = vfread(&sample, 1, 1, input) == 1;
		if (!success) {
			ConversionError("unexpected end of lump, "
			                "only %d samples read", i);
			break;
		}
		sample16 = (((short) sample) - 128) * 256;
		assert(sf_writef_short(out, &sample16, 1) == 1);
	}

	vfclose(input);
	sf_close(out);

	vfseek(result, 0, SEEK_SET);

	if (!success) {
		vfclose(result);
		result = NULL;
	}

	return result;
}
