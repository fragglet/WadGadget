//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "fs/vfile.h"
#include "pager/pager.h"
#include "pager/help.h"
#include "pager/hexdump.h"
#include "pager/plaintext.h"
#include "ui/dialog.h"

// Record lengths for different lump types. TODO: This should probably be
// done by lump type (lump_info.h), not by name.
static const struct {
	const char *lump_name;
	int record_length;
} lump_types[] = {
	{"PLAYPAL",  256 * 3},
	{"COLORMAP", 256},
	{"ENDOOM",   80 * 2},
	{"THINGS",   10},
	{"LINEDEFS", 14},
	{"LINEDEFS", 16},  // Hexen / Doom 64
	{"SIDEDEFS", 30},
	{"SIDEDEFS", 12},  // Doom 64
	{"SECTORS",  26},
	{"SECTORS",  28},  // PSX
	{"SECTORS",  16},  // PSX Final Doom
	{"SECTORS",  24},  // Doom 64
	{"VERTEXES",  4},
	{"SSECTORS",  4},
	{"NODES",    28},
	{"SEGS",     12},
};

static bool CanShowMarkers(struct hexdump_pager_config *cfg)
{
	// Number of columns must be a factor of the record length (each
	// record split over one or more lines), or vice versa (multiple
	// records per line).
	return cfg->record_length > 0
	    && (cfg->columns % cfg->record_length == 0
	     || cfg->record_length % cfg->columns == 0);
}

static void PrintRecordNumber(struct hexdump_pager_config *cfg, WINDOW *win,
                              unsigned int line)
{
	char buf[16];
	int record, end_record;

	if (!CanShowMarkers(cfg)) {
		return;
	}

	// Only show at first line of record.
	if ((line * cfg->columns) % cfg->record_length != 0) {
		return;
	}

	record = (line * cfg->columns) / cfg->record_length;
	snprintf(buf, sizeof(buf), "#%d", record);

	mvwaddstr(win, 0, cfg->columns * 4 + 14, buf);

	end_record = min((line + 1) * cfg->columns, cfg->data_len)
	           / cfg->record_length - 1;
	if (end_record > record) {
		snprintf(buf, sizeof(buf), "-%d", end_record);
		waddstr(win, buf);
	}
}

static void DrawHexdumpLine(WINDOW *win, unsigned int line, void *user_data)
{
	struct hexdump_pager_config *cfg = user_data;
	char buf[12];
	int i, b;

	wattron(win, A_BOLD);
	snprintf(buf, sizeof(buf), " %08x: ", line * cfg->columns);
	waddstr(win, buf);
	wattroff(win, A_BOLD);

	b = line * cfg->columns;
	for (i = 0; i < cfg->columns && b < cfg->data_len; ++i, ++b) {
		if (cfg->record_length > 0 && b % cfg->record_length == 0) {
			wattron(win, A_BOLD);
		}
		snprintf(buf, sizeof(buf), "%02x ",
		         cfg->data[line * cfg->columns + i]);
		waddstr(win, buf);
		wattroff(win, A_BOLD);
	}

	mvwaddstr(win, 0, cfg->columns * 3 + 12, "");

	b = line * cfg->columns;
	for (i = 0; i < cfg->columns && b < cfg->data_len; ++i, ++b) {
		int c = cfg->data[b];

		if (c >= 32 && c < 127) {
			waddch(win, c);
		} else {
			waddch(win, '.');
		}
	}

	PrintRecordNumber(cfg, win, line);
}

static void SwitchToASCII(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;
	struct plaintext_pager_config *ptc = cfg->plaintext_config;

	if (ptc == NULL) {
		VFILE *in = vfopenmem(cfg->data, cfg->data_len);
		ptc = checked_calloc(1, sizeof(struct plaintext_pager_config));
		assert(P_InitPlaintextConfig(cfg->pc.title, false, ptc, in));
		cfg->plaintext_config = ptc;
		ptc->hexdump_config = cfg;
	}

	P_SwitchConfig(&ptc->pc);
}

const struct action switch_ascii_action = {
	0, 'D', "ASCII", "View as ASCII", SwitchToASCII,
};

static int PagerWidth(struct hexdump_pager_config *cfg)
{
	int result = 15 + cfg->columns * 4;
	// Extra space needed for record counter?
	if (CanShowMarkers(cfg)) {
		result += 5;
		// Two or more records per line?
		if (cfg->columns > cfg->record_length) {
			result += 5;
		}
	}
	return result;
}

static void UpdateLineCount(struct hexdump_pager_config *cfg)
{
	cfg->pc.num_lines = (cfg->data_len + cfg->columns - 1) / cfg->columns;
}

static void SetDefaultColumns(struct hexdump_pager_config *cfg)
{
	cfg->columns = 256;
	while (PagerWidth(cfg) > COLS) {
		cfg->columns /= 2;
	}
}

static void SetColumns(struct hexdump_pager_config *cfg)
{
	if (cfg->record_length == 0) {
		SetDefaultColumns(cfg);
		UpdateLineCount(cfg);
		return;
	}

	// We need to reduce the number of columns so that there is
	// space on the right for the record counter. Then we may
	// need to reduce further to ensure that the column count
	// is either a factor or multiple of the record length.
	cfg->columns = COLS / 4;
	while (PagerWidth(cfg) > COLS ||!CanShowMarkers(cfg)) {
		--cfg->columns;
	}
	// Prime number?
	if (cfg->columns == 1) {
		SetDefaultColumns(cfg);
	}
}

static void ChangeRecordLength(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;
	char *answer, curr_buf[32] = "";
	int len, old_cols;

	if (cfg->record_length > 0) {
		snprintf(curr_buf, sizeof(curr_buf), "\n(Currently: %d bytes)",
		         cfg->record_length);
	}

	answer = UI_TextInputDialogBox(
		"Change record length", "Set length", 3,
		"Enter the number of bytes per record:%s", curr_buf);
	if (answer == NULL) {
		return;
	}

	len = atoi(answer);
	if (len <= 0) {
		cfg->record_length = 0;
		return;
	}

	old_cols = cfg->columns;
	cfg->record_length = len;
	SetColumns(cfg);
	current_pager->window_offset =
		current_pager->window_offset * old_cols / cfg->columns;

	UpdateLineCount(cfg);
}

const struct action change_record_length_action = {
	0, 'R', "RecordLen", "Record Length", ChangeRecordLength,
};

static void ChangeColumns(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;
	char *answer;
	int cols;

	answer = UI_TextInputDialogBox(
		"Change columns per line", "Set columns", 2,
		"Enter the number of columns\nto display per line:");
	if (answer == NULL) {
		return;
	}

	cols = atoi(answer);
	if (cols <= 0) {
		SetColumns(cfg);
		return;
	}

	current_pager->window_offset =
		current_pager->window_offset * cfg->columns / cols;

	cfg->columns = cols;
	UpdateLineCount(cfg);
}

const struct action change_columns_action = {
	0, 'O', "Columns", "Columns", ChangeColumns,
};

static void OpenDoomSpecs(void)
{
	struct hexdump_pager_config *cfg = current_pager->cfg->user_data;

	if (cfg->specs_help.pc.title == NULL) {
		if (!P_InitHelpConfig(&cfg->specs_help, "uds.md")) {
			cfg->specs_help.pc.title = NULL;
			return;
		}
		P_InitPager(&cfg->specs_pager, &cfg->specs_help.pc);
	}
	P_RunPager(&cfg->specs_pager);
}

const struct action open_specs_action = {
	0, 'U', "Specs", "Open Doom Specs", OpenDoomSpecs,
};

static const struct action *hexdump_pager_actions[] = {
	&exit_pager_action,
	&pager_help_action,
	&switch_ascii_action,
	&change_columns_action,
	&change_record_length_action,
	&pager_search_action,
	&pager_search_again_action,
	&open_specs_action,
	NULL,
};

static void SetBytesPerRecord(struct hexdump_pager_config *cfg,
                              const char *lump_name)
{
	int i;

	for (i = 0; i < arrlen(lump_types); i++) {
		int bpr = lump_types[i].record_length;
		if (!strcasecmp(lump_name, lump_types[i].lump_name)
		 && cfg->data_len % bpr == 0) {
			cfg->record_length = bpr;
			return;
		}
	}

	cfg->record_length = 0;
}

bool P_InitHexdumpConfig(const char *title, struct hexdump_pager_config *cfg,
                         VFILE *input)
{
	cfg->pc.title = title;
	cfg->pc.help_file = "hexdump.md";
	cfg->pc.draw_line = DrawHexdumpLine;
	cfg->pc.user_data = cfg;
	cfg->pc.actions = hexdump_pager_actions;
	cfg->pc.get_link = NULL;
	cfg->plaintext_config = NULL;
	cfg->specs_help.pc.title = NULL;

	cfg->data = vfreadall(input, &cfg->data_len);
	vfclose(input);

	SetBytesPerRecord(cfg, title);
	SetColumns(cfg);

	cfg->pc.num_links = 0;
	cfg->pc.current_link = -1;
	cfg->pc.current_column = 0;

	return cfg->data != NULL;
}

void P_FreeHexdumpConfig(struct hexdump_pager_config *cfg)
{
	free(cfg->data);
	if (cfg->specs_help.pc.title != NULL) {
		P_FreeHelpConfig(&cfg->specs_help);
		P_FreePager(&cfg->specs_pager);
	}
}

bool P_RunHexdumpPager(const char *title, VFILE *input)
{
	struct pager p;
	struct hexdump_pager_config cfg;

	if (!P_InitHexdumpConfig(title, &cfg, input)) {
		return false;
	}

	P_InitPager(&p, &cfg.pc);
	P_RunPager(&p);
	P_FreePager(&p);
	if (cfg.plaintext_config != NULL) {
		P_FreePlaintextConfig(cfg.plaintext_config);
	}
	P_FreeHexdumpConfig(&cfg);

	return true;
}
