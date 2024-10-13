//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifndef CONV__AUDIO_H_INCLUDED
#define CONV__AUDIO_H_INCLUDED

#include <stdint.h>

#include "fs/vfile.h"

struct sound_header {
	uint16_t format;
	uint16_t sample_rate;
	uint32_t num_samples;
};

VFILE *S_FromAudioFile(VFILE *input);
VFILE *S_ToAudioFile(VFILE *input);
void S_SwapSoundHeader(struct sound_header *hdr);

#endif /* #ifndef CONV__AUDIO_H_INCLUDED */
