//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <af_vfs.h>
#include <audiofile.h>
#include <assert.h>

#include "audio.h"
#include "common.h"

void S_SwapSoundHeader(struct sound_header *hdr)
{
	SwapLE16(&hdr->format);
	SwapLE16(&hdr->sample_rate);
	SwapLE32(&hdr->num_samples);
};

static ssize_t AudioFileRead(AFvirtualfile *vfile, void *data, size_t nbytes)
{
	return vfread(data, 1, nbytes, vfile->closure);
}

static AFfileoffset AudioFileLength(AFvirtualfile *vfile)
{
	VFILE *f = vfile->closure;
	AFfileoffset result, old_pos;

	old_pos = vftell(f);
	assert(vfseek(f, 0, SEEK_END) >= 0);
	result = vftell(f);
	assert(vfseek(f, old_pos, SEEK_SET) >= 0);

	return result;
}

static ssize_t AudioFileWrite(AFvirtualfile *vfile, const void *data,
                              size_t nbytes)
{
	return vfwrite(data, 1, nbytes, vfile->closure);
}

static void AudioFileDestroy(AFvirtualfile *vfile)
{
	//vfclose(vfile->closure);
}

static AFfileoffset AudioFileSeek(AFvirtualfile *vfile, AFfileoffset offset,
                           int is_relative)
{
	vfseek(vfile->closure, offset, is_relative ? SEEK_CUR : SEEK_SET);
	return vftell(vfile->closure);
}

static AFfileoffset AudioFileTell(AFvirtualfile *vfile)
{
	return vftell(vfile->closure);
}

static AFvirtualfile *MakeVirtualFile(VFILE *vf)
{
	AFvirtualfile *af = af_virtual_file_new();

	af->read = AudioFileRead;
	af->length = AudioFileLength;
	af->write = AudioFileWrite;
	af->destroy = AudioFileDestroy;
	af->seek = AudioFileSeek;
	af->tell = AudioFileTell;
	af->closure = vf;

	return af;
}

VFILE *S_FromAudioFile(VFILE *input)
{
	AFvirtualfile *avf = MakeVirtualFile(input);
	VFILE *result = vfopenmem(NULL, 0);
	AFfilehandle af = afOpenVirtualFile(avf, "r", AF_NULL_FILESETUP);
	struct sound_header hdr;

	if (af == NULL) {
		goto fail;
	}

	afSetVirtualChannels(af, AF_DEFAULT_TRACK, 1);
	afSetVirtualSampleFormat(af, AF_DEFAULT_TRACK, AF_SAMPFMT_UNSIGNED, 8);

	hdr.format = 3;
	hdr.sample_rate = (int) afGetRate(af, AF_DEFAULT_TRACK);
	hdr.num_samples = afGetFrameCount(af, AF_DEFAULT_TRACK);
	S_SwapSoundHeader(&hdr);
	vfwrite(&hdr, sizeof(hdr), 1, result);

	for (;;) {
		uint8_t buf[512];
		int frames = afReadFrames(af, AF_DEFAULT_TRACK, buf, sizeof(buf));

		if (frames < 0) {
			goto fail;
		} else if (frames == 0) {
			break;
		}

		vfwrite(buf, 1, frames, result);
	}

	vfseek(result, 0, SEEK_SET);

	goto end;

fail:
	vfclose(result);
	result = NULL;

end:
	if (af != NULL) {
		afCloseFile(af);
	} else {
		af_virtual_file_destroy(avf);
	}
	vfclose(input);

	return result;
}

VFILE *S_ToAudioFile(VFILE *input)
{
	struct sound_header hdr;
	AFfilesetup setup = afNewFileSetup();
	VFILE *result = vfopenmem(NULL, 0);
	AFvirtualfile *avf;
	AFfilehandle af;

	assert(vfread(&hdr, sizeof(hdr), 1, input) == 1);
	S_SwapSoundHeader(&hdr);
	assert(hdr.format == 3);

	afInitFileFormat(setup, AF_FILE_WAVE);
	afInitChannels(setup, AF_DEFAULT_TRACK, 1);
	afInitRate(setup, AF_DEFAULT_TRACK, hdr.sample_rate);
	afInitSampleFormat(setup, AF_DEFAULT_TRACK, AF_SAMPFMT_UNSIGNED, 8);

	avf = MakeVirtualFile(result);
	af = afOpenVirtualFile(avf, "w", setup);
	if (af == NULL) {
		goto fail;
	}

	for (;;) {
		uint8_t buf[512];
		int frames = vfread(buf, 1, sizeof(buf), input);

		if (frames < 0) {
			goto fail;
		} else if (frames == 0) {
			break;
		}

		if (afWriteFrames(af, AF_DEFAULT_TRACK, buf, frames) < 0) {
			goto fail;
		}
	}

	goto end;

fail:
	vfclose(result);
	result = NULL;

end:
	if (af != NULL) {
		afCloseFile(af);
	} else {
		af_virtual_file_destroy(avf);
	}
	vfclose(input);
	afFreeFileSetup(setup);

	if (result != NULL) {
		vfseek(result, 0, SEEK_SET);
	}

	return result;
}
