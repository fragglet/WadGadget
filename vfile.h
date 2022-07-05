
#ifndef INCLUDED_VFILE_H
#define INCLUDED_VFILE_H

#include <stdio.h>

typedef struct _VFILE VFILE;

struct vfile_functions {
	size_t (*read)(void *ptr, size_t size, size_t nitems, void *handle);
	size_t (*write)(const void *ptr, size_t size,
	                size_t nitems, void *handle);

	int (*seek)(void *handle, long offset);
	long (*tell)(void *handle);
	void (*close)(void *handle);
};

VFILE *vfopen(void *handle, struct vfile_functions *funcs);
VFILE *vfrestrict(VFILE *inner, long start, long end, int ro);
VFILE *vfwrapfile(FILE *stream);

void vfonclose(VFILE *stream, void (*callback)(VFILE *, void *), void *data);

size_t vfread(void *ptr, size_t size, size_t nitems, VFILE *stream);
size_t vfwrite(const void *ptr, size_t size, size_t nitems, VFILE *stream);

int vfseek(VFILE *stream, long offset);
long vftell(VFILE *stream);

void vfclose(VFILE *stream);

#endif /* #ifndef INCLUDED_VFILE_H */

