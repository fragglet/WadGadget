//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#ifdef HAVE_LIBSIXEL

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/time.h>

#include <sixel.h>

#include "termfuncs.h"
#include "sixel_display.h"
#include "stringlib.h"

#define SEND_ATTRIBUTES_ESCAPE  "\x1b[c"

static bool sixels_available = false;

bool SIXEL_CheckSupported(void)
{
	struct timeval start;
	struct saved_flags saved;
	bool result = false;
	char response[64];
	char *opt;
	int c, response_len;

	// Send vt220 SEND DEVICE ATTRIBUTES request.
	write(1, SEND_ATTRIBUTES_ESCAPE, strlen(SEND_ATTRIBUTES_ESCAPE));

	// The terminal will write the response to stdin. We need to read
	// it back, but we have to set some special options to be able to
	// read as intended.
	TF_SetRawMode(&saved, false);

	gettimeofday(&start, NULL);

	// Scan for the CSI escape sequence (0x1b).
	for (;;) {
		c = TF_PollingReadChar(&start);
		if (c == 0) {
			perror("SIXEL_CheckSupported");
			goto done;
		}
		if (c == 0x1b) {
			break;
		}
	}

	// Now save the body of the response to the buffer. The sequence
	// ends in a 'c'.
	response_len = 0;
	for (;;) {
		c = TF_PollingReadChar(&start);
		if (c == 0) {
			perror("SIXEL_CheckSupported");
			goto done;
		}
		if (c == 'c') {
			break;
		}
		if (response_len >= sizeof(response) - 1) {
			goto done;
		}
		response[response_len] = c;
		++response_len;
	}

	response[response_len] = '\0';

	// Split response into substrings, and look for "4" which indicates
	// that sixels are supported. We skip the first part (device class)
	strtok(response, ";");
	while ((opt = strtok(NULL, ";")) != NULL) {
		if (!strcmp(opt, "4")) {
			result = true;
			goto done;
		}
	}

done:
	TF_RestoreNormalMode(&saved);
	sixels_available = result;
	return result;
}

void SIXEL_ClearAndPrint(const char *msg, ...)
{
	va_list args;

	TF_ClearScreen();

	va_start(args, msg);
	vprintf(msg, args);
	va_end(args);
}

static int PromptUser(void)
{
	struct saved_flags saved;
	int result;

	printf("Press enter to continue, or 'E' to edit: ");
	fflush(stdout);

	TF_SetRawMode(&saved, true);
	for (result = 0; result != '\n' && result != 'e';) {
		result = getchar();
		result = tolower(result);
	}
	TF_RestoreNormalMode(&saved);

	return result;
}

bool SIXEL_DisplayImage(const char *filename)
{
	SIXELSTATUS status = SIXEL_FALSE;
	sixel_encoder_t *encoder;
	const char *scale;
	bool result;

	if (!sixels_available) {
		return false;
	}

	status = sixel_encoder_new(&encoder, NULL);
	if (SIXEL_FAILED(status)) {
		return false;
	}

	// We scale up the graphics, otherwise they look very small,
	// especially in iTerm on a retina display.
	// We don't scale up the Hexen startup screen though.
	if (StringHasSuffix(filename, ".hires.png")) {
		scale = "100%";
	} else {
		scale = "200%";
	}
	sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_WIDTH, scale);
	sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_HEIGHT, scale);

	// Blur-o-vision forever.
	sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_RESAMPLING, "nearest");

	status = sixel_encoder_encode(encoder, filename);
	sixel_encoder_unref(encoder);

	puts("");

	if (SIXEL_FAILED(status)) {
		return false;
	}

	// "Edit" command simulates a failure in displaying the image, in
	// which case we'll fall through to opening in another program.
	result = PromptUser() != 'e';

	// Clear screen before returning; this means that when we switch
	// back (eg. to display another image), the terminal won't have to
	// briefly display the image all over again.
	TF_ClearScreen();

	return result;
}

#else

#include <stdbool.h>

bool SIXEL_CheckSupported(void)
{
	return false;
}

void SIXEL_ClearAndPrint(const char *msg, ...)
{
}

bool SIXEL_DisplayImage(const char *filename)
{
	return false;
}

#endif

