#include "unimp.h"

struct ftime {};
#define S_IREAD 0
#define S_IWRITE 0

static int  _read(int h, void *b, unsigned l) { UNIMP; return 0; }
static int  _write(int h, const void *b, unsigned l) { UNIMP; return 0; }
static int  chmod(const char *p, int m) { UNIMP; return 0; }
static int  getftime(int h, struct ftime *f) { UNIMP; return 0; }
static int  setftime(int h, struct ftime *f) { UNIMP; return 0; }
static long filelength(int h) { UNIMP; return 0; }

