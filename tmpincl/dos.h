#include "unimp.h"

#define FP_OFF( fp ) 0
#define FP_SEG( fp ) 0
#define MK_FP( seg,ofs ) NULL
static int unlink(const char *__path ) { UNIMP; return 0; }
static char peekb( unsigned seg, unsigned off ) { UNIMP; return 0; }
static int pokeb(unsigned s, unsigned o, char v) { UNIMP; return 0; }


