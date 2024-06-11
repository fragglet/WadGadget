//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

struct lump_dir_funcs {
	struct pnames *(*get_pnames)(void *dir);
	bool (*load)(void *dir, struct directory *wad_dir,
	             struct directory_entry *ent);
	bool (*save)(void *dir, struct directory *wad_dir,
	             struct directory_entry *ent);
	VFILE *(*format_config)(void *dir, struct file_set *selected);
};

struct lump_dir {
	struct directory dir;
	const struct lump_dir_funcs *lump_dir_funcs;

	// Parent directory; always a WAD file.
	struct directory *parent_dir;
	uint64_t lump_serial;
};

struct directory *TX_LumpDirOpenDir(void *_dir,
                                    struct directory_entry *ent);
struct directory *TX_DirGetParent(struct directory *_dir,
                                  struct directory_entry **ent);
bool TX_DirReload(struct directory *dir);
bool TX_DirSave(struct directory *dir);
void TX_LumpDirFree(struct lump_dir *dir);
struct pnames *TX_GetDirPnames(struct directory *dir);
bool TX_InitLumpDir(struct lump_dir *dir, const struct lump_dir_funcs *funcs,
                    struct directory *parent, struct directory_entry *ent);
size_t TX_TextureLen(size_t patchcount);
