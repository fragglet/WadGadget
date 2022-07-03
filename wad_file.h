
struct wad_file;

struct wad_file_header {
	char id[4];
	unsigned int num_lumps;
	unsigned int table_offset;
};

struct wad_file_entry {
	unsigned int position;
	unsigned int size;
	char name[8];
};

struct wad_file *W_OpenFile(const char *file);
void W_CloseFile(struct wad_file *f);
struct wad_file_entry *W_GetDirectory(struct wad_file *f);
unsigned int W_NumLumps(struct wad_file *f);
