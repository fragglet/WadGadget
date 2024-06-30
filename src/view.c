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
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "actions.h"
#include "common.h"
#include "ui/dialog.h"
#include "conv/endoom.h"
#include "conv/error.h"
#include "conv/export.h"
#include "conv/import.h"
#include "lump_info.h"
#include "sixel_display.h"
#include "stringlib.h"
#include "termfuncs.h"
#include "ui/title_bar.h"
#include "ui/ui.h"
#include "fs/vfile.h"
#include "fs/vfs.h"

#ifndef _WIN32

static bool got_tstp;

static void RedrawScreen(void)
{
	clearok(stdscr, TRUE);
	wrefresh(stdscr);
	UI_DrawAllPanes();
}

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

// In all honesty, the Windows spawnv() API is much more convenient an API
// than the Unix fork/exec. For simplicity for porting for Windows, let's
// just emulate it where we don't have it.
#define _P_WAIT 1
static intptr_t _spawnv(int mode, const char *cmdname, char **argv)
{
	pid_t pid = fork();
	if (pid == -1) {
		return -1;
	} else if (pid == 0) {
		execvp(cmdname, argv);
		exit(-1);
	} else {
		return WaitSubprocess(pid);
	}
}
#endif

#ifndef __APPLE__
static bool CheckHaveXdgUtils(void)
{
	return system("xdg-open --version >/dev/null 2>&1") == 0;
}
#endif

struct temp_edit_context {
	char *temp_dir;
	char *filename;
	struct directory *from;
	struct directory_entry *ent;
	const struct lump_type *lt;
	int lumpnum;
	time_t orig_time;
};

static time_t ReadFileTime(const char *filename)
{
	struct stat s;

	if (stat(filename, &s) != 0) {
		return 0;
	}

	return s.st_mtime;
}

static char *TempExport(struct temp_edit_context *ctx, struct directory *from,
                        struct directory_entry *ent)
{
	char *temp_dir;

	ClearConversionErrors();

	ctx->from = from;
	ctx->ent = ent;

	temp_dir = getenv("TEMP");
	if (temp_dir == NULL) {
		temp_dir = "/tmp";
	}
	ctx->temp_dir = StringJoin("/", temp_dir, "wadgadget-XXXXXX", NULL);
	ctx->temp_dir = mkdtemp(ctx->temp_dir);

	ctx->lumpnum = ent - from->entries;
	ctx->lt = LI_IdentifyLump(VFS_WadFile(from), ctx->lumpnum);
	// TODO: If lt == &lump_type_level, export the whole level to a
	// temp file so we can edit it in a level editor.

	ctx->filename = StringJoin("", ctx->temp_dir, "/", ent->name,
	                           LI_GetExtension(ctx->lt, true), NULL);

	if (!ExportToFile(ctx->from, ctx->ent, ctx->lt, ctx->filename, true)) {
		UI_MessageBox("Failed to export to temp file:\n%s",
		              GetConversionError());
		free(ctx->filename);
		rmdir(ctx->temp_dir);
		free(ctx->temp_dir);
		ctx->temp_dir = NULL;
		return NULL;
	}

	ctx->orig_time = ReadFileTime(ctx->filename);

	return ctx->filename;
}

static bool TempFileChanged(struct temp_edit_context *ctx)
{
	time_t modtime;

	if (ctx->temp_dir == NULL) {
		return false;
	}

	modtime = ReadFileTime(ctx->filename);
	return modtime != 0 && ctx->orig_time != 0 && modtime > ctx->orig_time;
}

static void RaiseUsToTop(void)
{
#ifdef __APPLE__
	static const struct {
		const char *env;
		const char *appname;
	} terms[] = {
		{"iTerm",            "iTerm"},
		{"Apple_Terminal",   "Terminal"},
		{"Hyper",            "Hyper"},
		{"Tabby",            "Tabby"},
		{"rio",              "rio"},
		// Add your favorite terminal here. Not Warp though.
	};
	const char *termprog = getenv("TERM_PROGRAM");
	const char *appname = NULL;
	int i;

	for (i = 0; termprog != NULL && i < arrlen(terms); i++) {
		if (strstr(termprog, terms[i].env) != NULL) {
			appname = terms[i].appname;
		}
	}

	// This is a rather gross hack, but it works. We use AppleScript to
	// bring the terminal back to the foreground.
	if (appname != NULL) {
		char buf[100];
		snprintf(buf, sizeof(buf), "osascript -e 'tell "
		         "application \"%s\" to activate' "
		         ">/dev/null 2>/dev/null", appname);
		system(buf);
		return;
	}
#endif

	// Otherwise, we can try the xterm method.
	TF_SendRaiseWindowOp();
}

// Called after a successful edit to (if appropriate) import the changed
// file back into the WAD.
static bool TempMaybeImport(struct temp_edit_context *ctx)
{
	VFILE *from_file;
	int do_import;

	if (ctx->temp_dir == NULL) {
		return true;
	}

	// If the edit command succeeded, there are two possibilities. One is
	// that the editor really has run its course, and we immediately prompt
	// the user if the file was changed. The second, if we invoked
	// something like the Gimp, is that the file was loaded into an already
	// running program in the background. If this happens, the command
	// exits immediately but the user may still be editing. To handle the
	// latter case, we return back to the normal browsing screen, but
	// continue to silently keep checking in the background to see if the file
	// changes. As soon as the user presses a key we give up and stop, but
	// if the file does get changed in the background we can still take the
	// opportunity to prompt.
	timeout(100);
	while (!TempFileChanged(ctx)) {
		int c = getch();
		if (c != ERR) {
			ungetch(c);
			timeout(-1);
			RedrawScreen();
			return true;
		}
	}
	timeout(-1);

	RaiseUsToTop();
	RedrawScreen();

	// At this point we know there is a change, but if the WAD file
	// is read-only, we can't import it back again.
	if (W_IsReadOnly(VFS_WadFile(ctx->from))) {
		UI_MessageBox("Cannot import changed file; the WAD\n"
		              "file is read-only.");
		// TODO: The user went to the trouble of saving an edited
		// file. Should we at least save a copy somewhere permanent?
		return true;
	}

	do_import = UI_ConfirmDialogBox(
		"Update WAD?", "Import", "Ignore",
		"File was changed. Import back into WAD?");
	if (!do_import) {
		return true;
	}

	// At this point the user wants to update the WAD, and the WAD file
	// is not read-only. But we might still need to do the IWAD change
	// confirmation first.
	if (!CheckReadOnly(ctx->from)) {
		return true;
	}

	// Things can be a bit confusing here because we reversed directions
	// compared to the original export. 'from' was the WAD and lump we
	// originally exported from, but now it becomes the 'to' that we're
	// importing back to.
	from_file = VFS_Open(ctx->filename);
	assert(from_file != NULL);

	ClearConversionErrors();

	if (!ImportFromFile(from_file, ctx->filename, ctx->from,
	                    ctx->lumpnum, true)) {
		return !UI_ConfirmDialogBox(
			"Error", "Edit", "Abort", "Import failed. "
			"Error:\n%s\n\nEdit file again?",
			GetConversionError());
	}
	VFS_CommitChanges(ctx->from, "import of '%s'", ctx->ent->name);
	UI_ShowNotice("'%s' updated.", ctx->ent->name);
	VFS_Refresh(ctx->from);

	return true;
}

static void TempCleanup(struct temp_edit_context *ctx)
{
	if (ctx->temp_dir == NULL) {
		return;
	}
	remove(ctx->filename);
	rmdir(ctx->temp_dir);
	free(ctx->temp_dir);
	// ctx->filename will be freed by OpenEntry().
}

static const char *text_file_extensions[] = {
	".txt", ".c", ".h", ".cfg", ".ini",
	".deh", ".bex", ".hhe", ".seh",  // Dehacked and variants
};

static char *GetOpenCommand(const char *filename)
{
	char *editor = getenv("EDITOR");
	const char *extn;

	// For files with text file extensions, always respect the standard
	// Unix EDITOR environment variable. This should always work even if
	// xdg-utils isn't installed.
	if (editor != NULL) {
		int i;

		for (i = 0; i < arrlen(text_file_extensions); i++) {
			extn = text_file_extensions[i];
			if (StringHasSuffix(filename, extn)) {
				return editor;
			}
		}
	}

#ifdef __APPLE__
	return "open";
#else
	{
		static bool have_xdg_utils = false;
		have_xdg_utils = have_xdg_utils || CheckHaveXdgUtils();

		if (!have_xdg_utils) {
			TF_SetCursesModes();
			UI_MessageBox("Sorry, can't open files; xdg-open "
			              "command not found.\nYou should install "
			              "the xdg-utils package.");
			return NULL;
		}
	}
	return "xdg-open";
#endif
}

void OpenDirent(struct directory *dir, struct directory_entry *ent)
{
	struct temp_edit_context temp_ctx = {NULL};
	bool displayed, edit_success = true;
	char *argv[3];

	if (ent->type == FILE_TYPE_FILE) {
		argv[1] = VFS_EntryPath(dir, ent);
	} else {
		argv[1] = TempExport(&temp_ctx, dir, ent);
		if (argv[1] == NULL) {
			return;
		}
	}

try_again:
	// Temporarily suspend curses until the subprogram returns.
	TF_SuspendCursesMode();

	displayed = false;

	if (StringHasSuffix(argv[1], ".png")) {
		SIXEL_ClearAndPrint("Contents of '%s':\n", ent->name);
		displayed = SIXEL_DisplayImage(argv[1]);
	} else if (StringHasSuffix(argv[1], ".lmp")
	        && ent->size == ENDOOM_SIZE) {
		ENDOOM_ShowFile(argv[1]);
		displayed = true;
	}

	if (!displayed) {
		argv[0] = GetOpenCommand(argv[1]);
		argv[2] = NULL;

		if (argv[0] != NULL) {
			printf("Opening %s '%s'...\n"
			       "Waiting until program terminates.\n"
			       "(^Z = stop waiting, continue in background)\n",
			       ent->type == FILE_TYPE_LUMP ? "lump" : "file",
			       ent->name);

			edit_success = _spawnv(_P_WAIT, argv[0], argv) == 0;
		}
	}

	// Restore the curses display which may have been trashed if another
	// curses program was opened.
	TF_SetCursesModes();
	RedrawScreen();

	if (edit_success && !TempMaybeImport(&temp_ctx)) {
		goto try_again;
	}

	TempCleanup(&temp_ctx);
	free(argv[1]);

	if (!edit_success) {
		UI_MessageBox("Failed executing command to open file,\n"
		              "or program exited with an error.");
	}
}
