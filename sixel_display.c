
#ifdef HAVE_LIBSIXEL

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>

#include <sixel.h>

#include "sixel_display.h"

#define SCALE "200%"
#define CLEAR_SCREEN_ESCAPE "\x1b[H\x1b[2J"
#define SEND_ATTRIBUTES_ESCAPE  "\x1b[c"
#define RESPONSE_TIMEOUT 1000 /* ms */

static bool sixels_available = false;

struct saved_flags {
	int fcntl_opts;
	struct termios termios;
};

static void SetRawMode(struct saved_flags *f, bool blocking)
{
	struct termios raw;

	// Don't block reads from stdin; we want to time out.
	if (!blocking) {
		f->fcntl_opts = fcntl(0, F_GETFL);
		fcntl(0, F_SETFL, f->fcntl_opts | O_NONBLOCK);
	}

	// Don't print the response, and don't wait for a newline.
	tcgetattr(0, &f->termios);
	raw = f->termios;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(0, TCSAFLUSH, &raw);
}

static void RestoreNormalMode(struct saved_flags *f)
{
	fcntl(0, F_SETFL, f->fcntl_opts);
	tcsetattr(0, TCSAFLUSH, &f->termios);
}

static int TimeDiffMs(struct timeval *a, struct timeval *b)
{
	return (a->tv_sec - b->tv_sec) * 1000
	     + (a->tv_usec - b->tv_usec) / 1000;
}

// Read a character from stdin, timing out if no response is received
// in RESPONSE_TIMEOUT milliseconds.
static int PollingReadChar(struct timeval *start)
{
	struct timeval now;
	int c;

	for (;;) {
		gettimeofday(&now, NULL);
		if (TimeDiffMs(&now, start) > RESPONSE_TIMEOUT) {
			errno = EWOULDBLOCK;
			return 0;
		}
		errno = 0;
		c = getchar();
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			usleep(1000);
			continue;
		} else if (errno != 0) {
			return 0;
		}
		return c;
	}
}

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
	SetRawMode(&saved, false);

	gettimeofday(&start, NULL);

	// Scan for the CSI escape sequence (0x1b).
	for (;;) {
		c = PollingReadChar(&start);
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
		c = PollingReadChar(&start);
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
	RestoreNormalMode(&saved);
	sixels_available = result;
	return result;
}

void SIXEL_ClearAndPrint(const char *msg, ...)
{
	va_list args;

	write(1, CLEAR_SCREEN_ESCAPE, strlen(CLEAR_SCREEN_ESCAPE));

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

	SetRawMode(&saved, true);
	for (result = 0; result != '\n' && result != 'e';) {
		result = getchar();
		result = tolower(result);
	}
	RestoreNormalMode(&saved);
	printf("\n\n");

	return result;
}

bool SIXEL_DisplayImage(const char *filename)
{
	SIXELSTATUS status = SIXEL_FALSE;
	sixel_encoder_t *encoder;

	if (!sixels_available) {
		return false;
	}

	status = sixel_encoder_new(&encoder, NULL);
	if (SIXEL_FAILED(status)) {
		return false;
	}

	// We scale up the graphics, otherwise they look very small,
	// especially in iTerm on a retina display.
	sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_WIDTH, SCALE);
	sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_HEIGHT, SCALE);

	// Blur-o-vision forever.
	sixel_encoder_setopt(encoder, SIXEL_OPTFLAG_RESAMPLING, "nearest");

	status = sixel_encoder_encode(encoder, filename);
	sixel_encoder_unref(encoder);

	if (SIXEL_FAILED(status)) {
		return false;
	}

	// "Edit" command simulates a failure in displaying the image, in
	// which case we'll fall through to opening in another program.
	return PromptUser() != 'e';
}

#else

bool SIXEL_CheckSupported(void)
{
	return false;
}

bool SIXEL_DisplayImage(const char *filename)
{
	return false;
}

#endif

