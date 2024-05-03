
#ifdef HAVE_LIBSIXEL

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>

#define SEND_ATTRIBUTES_ESCAPE  "\x1b[c"
#define RESPONSE_TIMEOUT 1000 /* ms */

struct saved_flags {
	int fcntl_opts;
	struct termios termios;
};

static void SetRawMode(struct saved_flags *f)
{
	struct termios raw;

	// Don't block reads from stdin; we want to time out.
	f->fcntl_opts = fcntl(0, F_GETFL);
	fcntl(0, F_SETFL, f->fcntl_opts | O_NONBLOCK);

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
	SetRawMode(&saved);

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
	return result;
}

#else

bool SIXEL_CheckSupported(void)
{
	return false;
}

#endif

