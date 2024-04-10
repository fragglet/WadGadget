
#ifndef INCLUDED_VFILE_H
#define INCLUDED_VFILE_H

#include <stdio.h>
#include <stdbool.h>

typedef struct _VFILE VFILE;

struct vfile_functions {
	size_t (*read)(void *ptr, size_t size, size_t nitems, void *handle);
	size_t (*write)(const void *ptr, size_t size,
	                size_t nitems, void *handle);

	int (*seek)(void *handle, long offset, int whence);
	long (*tell)(void *handle);
	void (*truncate)(void *handle);
	void (*close)(void *handle);
	void (*sync)(void *handle);
};

VFILE *vfopen(void *handle, struct vfile_functions *funcs);
VFILE *vfrestrict(VFILE *inner, long start, long end, int ro);
VFILE *vfwrapfile(FILE *stream);

int vfcopy(VFILE *from, VFILE *to);

void vfonclose(VFILE *stream, void (*callback)(VFILE *, void *), void *data);

size_t vfread(void *ptr, size_t size, size_t nitems, VFILE *stream);
size_t vfwrite(const void *ptr, size_t size, size_t nitems, VFILE *stream);

// Truncates at current position (unlike ftruncate).
void vftruncate(VFILE *stream);

// Read/write to memory buffer.
VFILE *vfopenmem(void *buf, size_t buf_len);
bool vfgetbuf(VFILE *f, void **buf, size_t *buf_len);

int vfseek(VFILE *stream, long offset, int whence);
long vftell(VFILE *stream);

void vfsync(VFILE *stream);
void vfclose(VFILE *stream);

#endif /* #ifndef INCLUDED_VFILE_H */

