struct ffblk {
	char ff_attrib;
	unsigned int ff_ftime;
	unsigned int ff_fdate;
	char ff_name[1];
};

static int findfirst(const char *p, struct ffblk *ff, int attr) { return 0; }
static int findnext(struct ffblk *ff) { return 0; }
static int chdir(const char *__path) { return 0; }
static int getcurdir(int __drive, char *__directory) { return 0; }
