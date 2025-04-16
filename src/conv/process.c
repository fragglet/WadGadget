//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include "conv/process.h"

#include <assert.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "conv/error.h"
#include "fs/vfile.h"

struct subprocess {
	int in, out, err;
	pid_t pid;
};

// CloseFileHandles is called before exec() to ensure no open file handles
// (besides the stdin/out/err we have configured) are passed to the child
// process.
static void CloseFileHandles(void)
{
	long max = sysconf(_SC_OPEN_MAX);
	int i;

	for (i = 3; i < max; ++i) {
		close(i);
	}
}

static bool SpawnSubprocess(struct subprocess *p, const char **cmd)
{
	int in[2], out[2], err[2];
	pid_t pid;

	if (pipe(in) != 0) {
		return false;
	}
	if (pipe(out) != 0) {
		goto fail1;
	}
	if (pipe(err) != 0) {
		goto fail2;
	}
	pid = fork();
	if (pid < 0) {
		goto fail3;
	}
	if (pid == 0) {
		dup2(in[0], 0);
		dup2(out[1], 1);
		dup2(err[1], 2);
		CloseFileHandles();
		assert(execvp(cmd[0], (char *const *) cmd) == 0);
	}

	close(in[0]);
	close(out[1]);
	close(err[1]);
	p->in = in[1];
	p->out = out[0];
	p->err = err[0];
	p->pid = pid;

	return true;

fail3:
	close(err[0]);
	close(err[1]);
fail2:
	close(out[0]);
	close(out[1]);
fail1:
	close(in[0]);
	close(in[1]);
	return false;
}

struct filter {
	struct subprocess p;
	VFILE *in;
	uint8_t buf[256];
	size_t buf_len;
	char error_output[256];
	size_t error_output_len;
	bool report_errors;
};

static void FeedSubprocess(struct filter *f)
{
	ssize_t cnt;

	if (f->buf_len == 0) {
		f->buf_len = vfread(f->buf, 1, sizeof(f->buf), f->in);
	}

	// Nothing more to write?
	if (f->buf_len == 0) {
		// printf("subprocess %d: closing stdin\r\n", f->p.pid);
		vfclose(f->in);
		close(f->p.in);
		f->in = NULL;
		return;
	}

	cnt = write(f->p.in, f->buf, f->buf_len);
	assert(cnt >= 0);

	f->buf_len -= cnt;
	memmove(f->buf, f->buf + cnt, f->buf_len);
}

static void AppendErrorOutput(struct filter *f)
{
	char buf[64];
	ssize_t nbytes = read(f->p.err, buf, sizeof(buf));

	// printf("subprocess %d: %d err bytes\r\n", f->p.pid, nbytes);
	assert(nbytes >= 0);

	nbytes = min(nbytes, sizeof(f->error_output) - f->error_output_len);
	memcpy(f->error_output + f->error_output_len, buf, nbytes);
	f->error_output_len += nbytes;
}

static size_t FilterRead(void *ptr, size_t size, size_t nitems, void *handle)
{
	struct filter *f = handle;
	struct pollfd fds[3];
	int nfds;

	assert(size == 1);

	for (;;) {
		fds[0].fd = f->p.out;
		fds[0].events = POLLIN | POLLHUP;

		fds[1].fd = f->p.err;
		fds[1].events = POLLIN | POLLHUP;
		fds[2].revents = 0;

		nfds = 2;

		if (f->in != NULL) {
			fds[2].fd = f->p.in;
			fds[2].events = POLLOUT;
			++nfds;
		}

		assert(poll(fds, nfds, -1) >= 0);

		if ((fds[1].revents & POLLIN) != 0) {
			AppendErrorOutput(f);
		}
		if ((fds[0].revents & POLLIN) != 0) {
			ssize_t result = read(f->p.out, ptr, nitems);
			assert(result >= 0);
			return result;
		}
		if ((fds[0].revents & POLLHUP) != 0) {
			return 0;
		}
		if ((fds[2].revents & POLLOUT) != 0) {
			FeedSubprocess(f);
		}
	}
}

static void FilterClose(void *handle)
{
	struct filter *f = handle;
	int status;

	if (f->in != NULL) {
		vfclose(f->in);
		close(f->p.in);
	}
	close(f->p.out);
	close(f->p.err);

	if (waitpid(f->p.pid, &status, 0) == f->p.pid &&
	    (!WIFEXITED(status) || WEXITSTATUS(status) != 0) &&
	    f->report_errors) {
		f->error_output_len =
		    min(f->error_output_len, sizeof(f->error_output) - 1);
		f->error_output[f->error_output_len] = '\0';
		ConversionError("Subprocess %d exited with status %d\n"
		                "%s\n",
		                f->p.pid, WEXITSTATUS(status), f->error_output);
	}
	free(f);
}

static const struct vfile_functions filter_funcs = {
    FilterRead, NULL, NULL, NULL, NULL, FilterClose, NULL,
};

VFILE *SpawnSubprocessFilter(VFILE *input, const char **cmd, bool report_errors)
{
	struct filter *f = checked_calloc(1, sizeof(struct filter));
	f->in = input;
	f->buf_len = 0;
	f->error_output_len = 0;
	f->report_errors = report_errors;

	if (!SpawnSubprocess(&f->p, cmd)) {
		free(f);
		vfclose(input);
		return NULL;
	}

	return vfopen(f, &filter_funcs);
}
