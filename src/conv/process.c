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

#include <poll.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "fs/vfile.h"

struct subprocess {
	int in, out;
	pid_t pid;
};

static bool SpawnSubprocess(struct subprocess *p, const char **cmd)
{
	int in[2], out[2];
	pid_t pid;

	if (pipe(in) != 0) {
		return false;
	}
	if (pipe(out) != 0) {
		goto fail1;
	}
	pid = fork();
	if (pid < 0) {
		goto fail2;
	}
	if (pid == 0) {
		close(in[1]);
		dup2(in[0], 0);
		close(out[0]);
		dup2(out[1], 1);
		assert(execvp(cmd[0], (char *const *) cmd) == 0);
	}

	close(in[0]);
	close(out[1]);
	p->in = in[1];
	p->out = out[0];
	p->pid = pid;

	return true;

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

static size_t FilterRead(void *ptr, size_t size, size_t nitems, void *handle)
{
	struct filter *f = handle;
	struct pollfd fds[2];
	int nfds = 1;

	assert(size == 1);

	for (;;) {
		fds[0].fd = f->p.out;
		fds[0].events = POLLIN | POLLHUP;
		fds[0].revents = 0;
		fds[1].revents = 0;
		nfds = 1;

		if (f->in != NULL) {
			fds[1].fd = f->p.in;
			fds[1].events = POLLOUT;
			++nfds;
		}

		assert(poll(fds, nfds, -1) >= 0);

		if ((fds[0].revents & POLLIN) != 0) {
			ssize_t result = read(f->p.out, ptr, nitems);
			assert(result >= 0);
			return result;
		}
		if ((fds[0].revents & POLLHUP) != 0) {
			return 0;
		}
		if ((fds[1].revents & POLLOUT) != 0) {
			FeedSubprocess(f);
		}
	}
}

static void FilterClose(void *handle)
{
	struct filter *f = handle;

	if (f->in != NULL) {
		vfclose(f->in);
		close(f->p.in);
	}
	close(f->p.out);

	waitpid(f->p.pid, NULL, 0);
	free(f);
}

static const struct vfile_functions filter_funcs = {
    FilterRead, NULL, NULL, NULL, NULL, FilterClose, NULL,
};

VFILE *SpawnSubprocessFilter(VFILE *input, const char **cmd)
{
	struct filter *f = checked_calloc(1, sizeof(struct filter));
	f->in = input;
	f->buf_len = 0;

	if (!SpawnSubprocess(&f->p, cmd)) {
		free(f);
		vfclose(input);
		return NULL;
	}

	return vfopen(f, &filter_funcs);
}
