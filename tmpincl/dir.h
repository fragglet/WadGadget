#include "unimp.h"

struct ffblk {
	char ff_attrib;
	unsigned int ff_ftime;
	unsigned int ff_fdate;
	char ff_name[1];
};

static int findfirst(const char *p, struct ffblk *ff, int attr) { UNIMP; return 0; }
static int findnext(struct ffblk *ff) { UNIMP; return 0; }
static int chdir(const char *__path) { UNIMP; return 0; }
static int getcurdir(int __drive, char *__directory) { UNIMP; return 0; }
