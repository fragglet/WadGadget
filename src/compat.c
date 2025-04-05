//
// Copyright(C) 2025 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
//
// POSIX compatibility functions for Windows build

#include "compat.h"

#ifdef _WIN32

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "stringlib.h"

ssize_t readlink(const char *restrict pathname, char *restrict buf,
                 size_t bufsiz)
{
	FILE *fs;
	size_t cnt;

	fs = fopen(pathname, "rb");
	if (fs == NULL) {
		return -1;
	}

	cnt = fread(buf, 1, bufsiz, fs);
	fclose(fs);

	if (cnt == 0) {
		return -1;
	}

	return cnt;
}

int symlink(const char *target, const char *linkpath)
{
	FILE *fs;
	size_t cnt;

	fs = fopen(linkpath, "wb");
	if (fs == NULL) {
		return -1;
	}

	cnt = fwrite(target, 1, strlen(target), fs);

	fclose(fs);

	return cnt == strlen(target) ? 0 : -1;
}

int setenv(const char *name, const char *value, int overwrite)
{
	size_t len = strlen(name) + strlen(value) + 2;
	char *envstring = checked_calloc(len, 1);
	int result;

	snprintf(envstring, len, "%s=%s", name, value);
	result = _putenv(envstring);
	free(envstring);

	return result;
}

int unsetenv(const char *name)
{
	size_t len = strlen(name) + 2;
	char *envstring = checked_calloc(len, 1);
	int result;

	snprintf(envstring, len, "%s=", name);
	result = _putenv(envstring);
	free(envstring);

	return result;
}

char *mkdtemp(char *name)
{
	static const char *random_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	size_t name_len = strlen(name);
	char *xx_part;
	int i;

	if (name_len < 6 || strcmp(name + name_len - 6, "XXXXXX") != 0) {
		errno = EINVAL;
		return NULL;
	}

	xx_part = name + name_len - 6;

	// Generate a random directory name and try to create it. It is always
	// possible that the random name already exists.
	do {
		for (i = 0; i < 6; ++i) {
			xx_part[i] = random_chars[rand() % 36];
		}
		if (_mkdir(name) == 0) {
			return name;
		}
	} while (errno == EEXIST);

	// Some other kind of error.
	return NULL;
}

#else  /* #ifndef _WIN32 */

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

static bool got_tstp;

// Handler function invoked when SIGTSTP (^Z) is received.
static void TstpHandler(int unused)
{
	// Allow the waitpid() loop below to exit; we don't want to continue
	// waiting for the subprogram to exit.
	got_tstp = true;

	// The subprocess(es) are part of the same process group, so they all
	// received a TSTP just like us. We want them to be able to continue
	// running, so send them a SIGCONT.
	kill(0, SIGCONT);
}

static void SuspendSignal(int signum, struct sigaction *saved)
{
	struct sigaction tmp;

	assert(sigaction(signum, NULL, saved) == 0);
	tmp = *saved;
	tmp.sa_handler = SIG_IGN;
	assert(sigaction(signum, &tmp, NULL) == 0);
}

static void RestoreSignal(int signum, struct sigaction *saved)
{
	assert(sigaction(signum, saved, NULL) == 0);
}

static intptr_t WaitSubprocess(pid_t pid)
{
	struct sigaction old_sigint, old_sigterm, old_sigwinch;
	struct sigaction tstp_action, old_sigtstp;
	int result, err;

	// We have special handling for the SIGTSTP signal. If the user
	// presses ^Z we stop waiting and let the editor keep running in the
	// background (useful if the program is a GUI app like the Gimp). To
	// do this, we have to install our own signal handler and disable
	// the SA_RESTART flag so that waitpid() below will return.
	assert(sigaction(SIGTSTP, NULL, &old_sigtstp) == 0);
	memset(&tstp_action, 0, sizeof(struct sigaction));
	tstp_action.sa_handler = TstpHandler;
	tstp_action.sa_mask = old_sigtstp.sa_mask;
	tstp_action.sa_flags = old_sigtstp.sa_flags & ~SA_RESTART;
	assert(sigaction(SIGTSTP, &tstp_action, &old_sigtstp) == 0);
	got_tstp = false;

	// We ignore SIGINT and others while waiting; the subprocess handles
	// it. This allows us to ^C the subcommand without exiting the
	// entire program.
	SuspendSignal(SIGINT, &old_sigint);
	SuspendSignal(SIGTERM, &old_sigterm);
	SuspendSignal(SIGWINCH, &old_sigwinch);

	// Keep restarting waitpid unless we receive a SIGTSTP.
	do {
		err = waitpid(pid, &result, 0);
	} while (err == EAGAIN && !got_tstp);

	RestoreSignal(SIGTSTP, &old_sigtstp);
	RestoreSignal(SIGINT, &old_sigint);
	RestoreSignal(SIGTERM, &old_sigterm);
	RestoreSignal(SIGWINCH, &old_sigwinch);

	if (got_tstp) {
		return 0;
	} else if (!WIFEXITED(result)) {
		return -1;
	} else {
		return WEXITSTATUS(result);
	}
}

// In a curious reversal, on non-Windows systems we instead *implement* the
// Win32 spawnv() function. In all honesty, it's a much more convenient API
// than the Unix fork/exec. For simplicity for porting for Windows, let's just
// emulate it where we don't have it, and pretend we have it everywhere.
intptr_t _spawnv(int mode, const char *cmdname, const char **argv)
{
	pid_t pid = fork();
	if (pid == -1) {
		return -1;
	} else if (pid == 0) {
		execvp(cmdname, (char **) argv);
		exit(-1);
	} else {
		return WaitSubprocess(pid);
	}
}

#endif
