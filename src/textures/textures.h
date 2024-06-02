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
	struct pnames *pnames;
};

struct texture *TX_AllocTexture(size_t patchcount);
struct texture *TX_DupTexture(struct texture *t);
struct texture *TX_AddPatch(struct texture *t, struct patch *p);

VFILE *TX_MarshalTextures(struct textures *txs);
struct textures *TX_UnmarshalTextures(VFILE *input);
void TX_FreePnames(struct pnames *t);
void TX_FreeTextures(struct textures *t);

VFILE *TX_MarshalPnames(struct pnames *pn);
struct pnames *TX_UnmarshalPnames(VFILE *f);
int TX_GetPnameIndex(struct pnames *pn, const char *name);

VFILE *TX_ToTexturesConfig(VFILE *input, VFILE *pnames_input);
VFILE *TX_ToPnamesConfig(VFILE *input);
VFILE *TX_FromTexturesConfig(VFILE *input, VFILE *pnames_input);

void TX_AddSerialNos(struct textures *txs);
