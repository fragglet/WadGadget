//
// Copyright(C) 2022-2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <curses.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "common.h"
#include "ui/directory_pane.h"
#include "browser/browser.h"
#include "sixel_display.h"
#include "termfuncs.h"
#include "ui/ui.h"

#define RESPONSE_FILE_PATH "/tmp/wadgadget-paths.txt"

#define VERSION_OUTPUT \
"WadGadget version ?\n" \
"Copyright (C) 2022-2024 Simon Howard\n" \
"License GPLv2+: GNU GPL version 2 or later:\n" \
"<https://www.gnu.org/licenses/old-licenses/gpl-2.0.html>\n" \
"\n" \
"This is free software; see COPYING.md for copying conditions. There is NO\n" \
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"

// We set a custom handler for SIGTSTP. This is the signal that is sent when
// the user types a Ctrl-Z. This allows us to use this key combo (for Undo).
static void TermStopHandler(int unused)
{
	ungetch(CTRL_('Z'));
}

static void SetTermStopHandler(void)
{
	struct sigaction sa;
	sigaction(SIGTSTP, NULL, &sa);
	sa.sa_handler = TermStopHandler;
	sa.sa_flags = sa.sa_flags & ~SA_RESTART;
	sigaction(SIGTSTP, &sa, NULL);
}

#ifdef __APPLE__
static char *NextLine(char **buf, size_t *buf_len)
{
	char *p = memchr(*buf, '\n', *buf_len);
	char *result = *buf;

	if (p == NULL) {
		return NULL;
	}

	*buf_len -= (p - *buf) + 1;
	*buf = p + 1;
	*p = '\0';

	return result;
}

// On macOS, we use a response file to pass through the initial paths
// from the launcher program to the main wadgadget binary. It's hacky
// but there doesn't appear to be a cleaner solution.
static void LoadResponseFile(int *argc, char ***argv)
{
	static char *replacement_argv[3];
	char *buf;
	size_t buf_len;
	FILE *fs;
	VFILE *vfs;

	fs = fopen(RESPONSE_FILE_PATH, "r");
	if (fs == NULL) {
		return;
	}

	vfs = vfwrapfile(fs);
	buf = vfreadall(vfs, &buf_len);
	vfclose(vfs);

	replacement_argv[0] = (*argv)[0];

	for (*argc = 1; *argc < arrlen(replacement_argv); ++*argc) {
		char *line = NextLine(&buf, &buf_len);
		if (line == NULL) {
			break;
		}
		replacement_argv[*argc] = line;
	}

	*argv = replacement_argv;

	// Next time we run the program we don't want to use the
	// response file again.
	remove(RESPONSE_FILE_PATH);
}
#endif

int main(int argc, char *argv[])
{
	const char *start_path1 = ".", *start_path2 = ".";

	SetTermStopHandler();
#ifdef SIGIO
	signal(SIGIO, SIG_IGN);
#endif
#ifdef SIGPOLL
	signal(SIGPOLL, SIG_IGN);
#endif

	SIXEL_CheckSupported();

	if (argc == 2 && !strcmp(argv[1], "--version")) {
		printf(VERSION_OUTPUT);
		exit(0);
	}

#ifdef __APPLE__
	LoadResponseFile(&argc, &argv);
#endif

	if (argc >= 2) {
		start_path1 = argv[1];
	}
	if (argc >= 3) {
		start_path2 = argv[2];
	}

	initscr();
	start_color();
	TF_SetNewPalette();
	TF_SetCursesModes();
	TF_SetColorPairs();

	refresh();

	UI_Init();
	B_Init(start_path1, start_path2);
	UI_RunMainLoop();

	B_Shutdown();
}
