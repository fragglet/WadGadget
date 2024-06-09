//
// Copyright(C) 2024 Simon Howard
//
// You can redistribute and/or modify this program under the terms of
// the GNU General Public License version 2 as published by the Free
// Software Foundation, or any later version. This program is
// distributed WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//

typedef char pname[8];

struct pnames {
	pname *pnames;
	size_t num_pnames;
	unsigned int modified_count;
};

struct patch {
	int16_t originx, originy;
	uint16_t patch;
	uint16_t stepdir;  // unused
	uint16_t colormap;  // unused
};

struct texture {
	char name[8];
	uint32_t masked;  // unused
	uint16_t width, height;
	uint32_t columndirectory;  // unused
	uint16_t patchcount;
	struct patch patches[1];
};

struct textures {
	struct texture **textures;
	size_t num_textures;
	uint64_t *serial_nos;
	unsigned int modified_count;
};

struct texture *TX_AllocTexture(size_t patchcount);
struct texture *TX_DupTexture(struct texture *t);
struct texture *TX_AddPatch(struct texture *t, struct patch *p);
struct texture *TX_TextureForName(struct textures *txs, const char *name);
bool TX_AddTexture(struct textures *txs, unsigned int pos, struct texture *t);
void TX_RemoveTexture(struct textures *txs, unsigned int idx);
bool TX_RenameTexture(struct textures *txs, unsigned int idx,
                      const char *new_name);
VFILE *TX_MarshalTextures(struct textures *txs);
struct textures *TX_UnmarshalTextures(VFILE *input);
void TX_FreeTextures(struct textures *t);

VFILE *TX_MarshalPnames(struct pnames *pn);
struct pnames *TX_UnmarshalPnames(VFILE *f);
int TX_AppendPname(struct pnames *pn, const char *name);
int TX_GetPnameIndex(struct pnames *pn, const char *name);
void TX_RemovePname(struct pnames *pn, unsigned int idx);
void TX_RenamePname(struct pnames *pn, unsigned int idx,
                    const char *name);
void TX_FreePnames(struct pnames *t);

VFILE *TX_FormatTexturesConfig(struct textures *txs, struct pnames *pn,
                               const char *comment);
VFILE *TX_FormatPnamesConfig(struct pnames *p);
struct textures *TX_ParseTextureConfig(VFILE *input, struct pnames *pn);
struct pnames *TX_ParsePnamesConfig(VFILE *input);

void TX_AddSerialNos(struct textures *txs);

struct directory *TX_OpenTextureDir(struct directory *parent,
                                    struct directory_entry *ent);
bool TX_DirReload(struct directory *_dir);
struct textures *TX_TextureList(struct directory *_dir);
struct directory *TX_DirGetParent(struct directory *_dir,
                                  struct directory_entry **ent);
struct pnames *TX_GetDirPnames(struct directory *_dir);
struct directory *TX_OpenPnamesDir(struct directory *parent,
                                   struct directory_entry *ent);

extern const struct action new_texture_action;
extern const struct action edit_textures_action;
extern const struct action edit_pnames_action;
extern const struct action dup_texture_action;
extern const struct action export_texture_config;
extern const struct action new_pname_action;
