//
// Copyright(C) 2022 Ryan Krafnick
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <curses.h>

#include "common.h"
#include "endoom.h"
#include "termfuncs.h"

static const char *cp437_to_utf8[256] = {
	" ", "\xe2\x98\xba", "\xe2\x98\xbb", "\xe2\x99\xa5",
	"\xe2\x99\xa6", "\xe2\x99\xa3", "\xe2\x99\xa0", "\xe2\x80\xa2",
	"\xe2\x97\x98", "\xe2\x97\x8b", "\xe2\x97\x99", "\xe2\x99\x82",
	"\xe2\x99\x80", "\xe2\x99\xaa", "\xe2\x99\xab", "\xe2\x98\xbc",

	"\xe2\x96\xba", "\xe2\x97\x84", "\xe2\x86\x95", "\xe2\x80\xbc",
	"\xc2\xb6", "\xc2\xa7", "\xe2\x96\xac", "\xe2\x86\xa8",
	"\xe2\x86\x91", "\xe2\x86\x93", "\xe2\x86\x92", "\xe2\x86\x90",
	"\xe2\x88\x9f", "\xe2\x86\x94", "\xe2\x96\xb2", "\xe2\x96\xbc",

	" ", "!", "\"", "#", "$", "%", "&", "'",
	"(", ")", "*", "+", ",", "-", ".", "/",

	"0", "1", "2", "3", "4", "5", "6", "7",
	"8", "9", ":", ";", "<", "=", ">", "?",

	"@", "A", "B", "C", "D", "E", "F", "G",
	"H", "I", "J", "K", "L", "M", "N", "O",

	"P", "Q", "R", "S", "T", "U", "V", "W",
	"X", "Y", "Z", "[", "\\", "]", "^", "_",

	"`", "a", "b", "c", "d", "e", "f", "g",
	"h", "i", "j", "k", "l", "m", "n", "o",

	"p", "q", "r", "s", "t", "u", "v", "w",
	"x", "y", "z", "{", "|", "}", "~", "\xe2\x8c\x82",

	"\xc3\x87", "\xc3\xbc", "\xc3\xa9", "\xc3\xa2",
	"\xc3\xa4", "\xc3\xa0", "\xc3\xa5", "\xc3\xa7",
	"\xc3\xaa", "\xc3\xab", "\xc3\xa8", "\xc3\xaf",
	"\xc3\xae", "\xc3\xac", "\xc3\x84", "\xc3\x85",

	"\xc3\x89", "\xc3\xa6", "\xc3\x86", "\xc3\xb4",
	"\xc3\xb6", "\xc3\xb2", "\xc3\xbb", "\xc3\xb9",
	"\xc3\xbf", "\xc3\x96", "\xc3\x9c", "\xc2\xa2",
	"\xc2\xa3", "\xc2\xa5", "\xe2\x82\xa7", "\xc6\x92",

	"\xc3\xa1", "\xc3\xad", "\xc3\xb3", "\xc3\xba",
	"\xc3\xb1", "\xc3\x91", "\xc2\xaa", "\xc2\xba",
	"\xc2\xbf", "\xe2\x8c\x90", "\xc2\xac", "\xc2\xbd",
	"\xc2\xbc", "\xc2\xa1", "\xc2\xab", "\xc2\xbb",

	"\xe2\x96\x91", "\xe2\x96\x92", "\xe2\x96\x93", "\xe2\x94\x82",
	"\xe2\x94\xa4", "\xe2\x95\xa1", "\xe2\x95\xa2", "\xe2\x95\x96",
	"\xe2\x95\x95", "\xe2\x95\xa3", "\xe2\x95\x91", "\xe2\x95\x97",
	"\xe2\x95\x9d", "\xe2\x95\x9c", "\xe2\x95\x9b", "\xe2\x94\x90",

	"\xe2\x94\x94", "\xe2\x94\xb4", "\xe2\x94\xac", "\xe2\x94\x9c",
	"\xe2\x94\x80", "\xe2\x94\xbc", "\xe2\x95\x9e", "\xe2\x95\x9f",
	"\xe2\x95\x9a", "\xe2\x95\x94", "\xe2\x95\xa9", "\xe2\x95\xa6",
	"\xe2\x95\xa0", "\xe2\x95\x90", "\xe2\x95\xac", "\xe2\x95\xa7",

	"\xe2\x95\xa8", "\xe2\x95\xa4", "\xe2\x95\xa5", "\xe2\x95\x99",
	"\xe2\x95\x98", "\xe2\x95\x92", "\xe2\x95\x93", "\xe2\x95\xab",
	"\xe2\x95\xaa", "\xe2\x94\x98", "\xe2\x94\x8c", "\xe2\x96\x88",
	"\xe2\x96\x84", "\xe2\x96\x8c", "\xe2\x96\x90", "\xe2\x96\x80",

	"\xce\xb1", "\xc3\x9f", "\xce\x93", "\xcf\x80",
	"\xce\xa3", "\xcf\x83", "\xc2\xb5", "\xcf\x84",
	"\xce\xa6", "\xce\x98", "\xce\xa9", "\xce\xb4",
	"\xe2\x88\x9e", "\xcf\x86", "\xce\xb5", "\xe2\x88\xa9",

	"\xe2\x89\xa1", "\xc2\xb1", "\xe2\x89\xa5", "\xe2\x89\xa4",
	"\xe2\x8c\xa0", "\xe2\x8c\xa1", "\xc3\xb7", "\xe2\x89\x88",
	"\xc2\xb0", "\xe2\x88\x99", "\xc2\xb7", "\xe2\x88\x9a",
	"\xe2\x81\xbf", "\xc2\xb2", "\xe2\x96\xa0", "\xc2\xa0",
};

static const char* color_lookup[] = {
	"0", "4", "2", "6", "1", "5", "3", "7",
	"0;1", "4;1", "2;1", "6;1", "1;1", "5;1", "3;1", "7;1",
};

static void PrintEndoom(uint8_t *endoom, bool use_utf8)
{
	uint8_t character, data;
	const char *foreground, *background, *blink, *cs;
	int i;

	TF_ClearScreen();

	for (i = 0; i < ENDOOM_SIZE / 2; ++i) {
		character = endoom[i * 2];
		data = endoom[i * 2 + 1];
		foreground = color_lookup[data & 0x0f];
		background = color_lookup[(data >> 4) & 0x07];
		blink = ((data >> 7) & 0x01) ? "5" : "25";
		cs = cp437_to_utf8[character];

		// Fallback for when UTF-8 isn't supported by terminal.
		if (!use_utf8 && strlen(cs) > 1) {
			cs = " ";
		}

		printf("\033[3%sm\033[4%sm\033[%sm%s\033[0m",
		       foreground, background, blink, cs);

		if ((i + 1) % 80 == 0 && (i + 1) != (ENDOOM_SIZE / 2)) {
			printf("\n");
		}
	}
}

static bool HaveUTF8(void)
{
	const char *p, *envs[] = {"LC_ALL", "LC_CTYPE", "LANG"};
	int i;

	for (i = 0; i < arrlen(envs); i++) {
		p = getenv(envs[i]);
		if (p != NULL && (strstr(p, "UTF-8") != NULL
		               || strstr(p, "UTF8") != NULL)) {
			return true;
		}
	}

	return false;
}

void ENDOOM_ShowFile(const char *filename)
{
	uint8_t *buf;
	size_t cnt;
	FILE *fs;

	fs = fopen(filename, "rb");
	if (fs == NULL) {
		perror("fopen");
		return;
	}

	buf = checked_malloc(ENDOOM_SIZE);
	cnt = fread(buf, 1, ENDOOM_SIZE, fs);
	fclose(fs);

	if (cnt == ENDOOM_SIZE) {
		PrintEndoom(buf, HaveUTF8());
	} else {
		fprintf(stderr, "Error: Only read %d bytes from %s\n",
		        cnt, filename);
	}

	free(buf);

	if (getmaxy(stdscr) >= 26) {
		printf("\nPress enter to continue. ");
		fflush(stdout);
	}

	while (getchar() != '\n') {
	}
}

#ifdef TEST
int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 1;
	}
	ENDOOM_ShowFile(argv[1]);
	return 0;
}
#endif
