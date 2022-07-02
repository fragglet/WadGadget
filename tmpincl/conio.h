#include <stdlib.h>
#include "unimp.h"

static int getch( void ) { UNIMP; return 0; }
static int kbhit( void ) { UNIMP; return 0; }
static int wherex( void ) { UNIMP; return 0; }
static int wherey( void ) { UNIMP; return 0; }
static void gotoxy( int x, int y ) { UNIMP; }

static char *itoa(int v, char *s, int radix) { UNIMP; return NULL; }
static char *strupr(char *s) { UNIMP; return NULL; }
static int stricmp(const char *s1, const char *s2) { UNIMP; return 0; }
static int strnicmp(const char *s1, const char *s2, size_t n) { UNIMP; return 0; }
static char *ltoa(long v, char *s, int r) { UNIMP; return NULL; }

