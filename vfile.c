
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include "common.h"
#include "vfile.h"

struct _VFILE {
	const struct vfile_functions *functions;
	void *handle;
	void (*onclose)(VFILE *, void *);
	void *onclose_data;
};

VFILE *vfopen(void *handle, struct vfile_functions *funcs)
{
	VFILE *result;
	result = checked_calloc(1, sizeof(VFILE));
	result->functions = funcs;
	result->handle = handle;
	return result;
}

size_t vfread(void *ptr, size_t size, size_t nitems, VFILE *stream)
{
	return stream->functions->read(ptr, size, nitems, stream->handle);
}

size_t vfwrite(const void *ptr, size_t size, size_t nitems, VFILE *stream)
{
	return stream->functions->write(ptr, size, nitems, stream->handle);
}

void vftruncate(VFILE *stream)
{
	return stream->functions->truncate(stream->handle);
}

int vfseek(VFILE *stream, long offset, int whence)
{
	return stream->functions->seek(stream->handle, offset, whence);
}

long vftell(VFILE *stream)
{
	return stream->functions->tell(stream->handle);
}

void vfsync(VFILE *stream)
{
	stream->functions->sync(stream->handle);
}

void vfclose(VFILE *stream)
{
	if (stream->onclose != NULL) {
		stream->onclose(stream, stream->onclose_data);
	}
	stream->functions->close(stream->handle);
	free(stream);
}

void vfonclose(VFILE *stream, void (*callback)(VFILE *, void *), void *data)
{
	stream->onclose = callback;
	stream->onclose_data = data;
}

static size_t wrapped_fread(void *ptr, size_t size, size_t nitems, void *handle)
{
	return fread(ptr, size, nitems, handle);
}

static size_t wrapped_fwrite(const void *ptr, size_t size,
                             size_t nitems, void *handle)
{
	return fwrite(ptr, size, nitems, handle);
}

static void wrapped_ftruncate(void *handle)
{
	ftruncate(fileno((FILE *) handle), ftell(handle));
}

static int wrapped_fseek(void *handle, long offset, int whence)
{
	return fseek(handle, offset, whence);
}

static long wrapped_ftell(void *handle)
{
	return ftell(handle);
}

static void wrapped_fsync(void *handle)
{
	fflush(handle);
	fsync(fileno((FILE *) handle));
}

static void wrapped_fclose(void *handle)
{
	fclose(handle);
}

static struct vfile_functions wrapped_io_functions = {
	wrapped_fread,
	wrapped_fwrite,
	wrapped_fseek,
	wrapped_ftell,
	wrapped_ftruncate,
	wrapped_fclose,
	wrapped_fsync,
};

VFILE *vfwrapfile(FILE *stream)
{
	return vfopen(stream, &wrapped_io_functions);
}

struct restricted_vfile {
	VFILE *inner;
	long start, end, pos;
	int ro;
};

static size_t restricted_vfread(void *ptr, size_t size, size_t nitems, void *handle)
{
	struct restricted_vfile *restricted = handle;
	size_t nreadable, result;

	if (restricted->end >= 0) {
		nreadable = (restricted->end - restricted->pos) / size;
		if (nitems > nreadable) {
			nitems = nreadable;
		}
	}
	if (nitems == 0) {
		return 0;
	}

	result = vfread(ptr, size, nitems, restricted->inner);
	if (result < 0) {
		return -1;
	}

	restricted->pos += result * size;
	return result;
}

static size_t restricted_fwrite(const void *ptr, size_t size,
                                size_t nitems, void *handle)
{
	struct restricted_vfile *restricted = handle;
	size_t nwriteable, result;

	if (restricted->ro) {
		return -1;
	}

	if (restricted->end >= 0) {
		nwriteable = (restricted->end - restricted->pos) / size;
		if (nitems > nwriteable) {
			nitems = nwriteable;
		}
	}
	if (nitems == 0) {
		return 0;
	}

	result = vfwrite(ptr, size, nitems, restricted->inner);
	if (result < 0) {
		return -1;
	}

	restricted->pos += result * size;
	return result;
}

static int restricted_vfseek(void *handle, long offset, int whence)
{
	struct restricted_vfile *restricted = handle;
	long adjusted_offset;

	assert(whence == SEEK_SET); // SEEK_{CUR,END} not implemented.

	adjusted_offset = offset + restricted->start;

	if (restricted->end >= 0 && adjusted_offset > restricted->end) {
		return -1;
	}

	if (vfseek(restricted->inner, adjusted_offset, whence) < 0) {
		return -1;
	}

	restricted->pos = offset;
	return 0;
}

static long restricted_vftell(void *handle)
{
	struct restricted_vfile *restricted = handle;
	return restricted->pos;
}

static void restricted_vftruncate(void *handle)
{
	// Can't truncate file within a restricted file?
	assert(0);
}

static void restricted_vfsync(void *handle)
{
	struct restricted_vfile *restricted = handle;
	vfsync(restricted->inner);
}

static void restricted_vfclose(void *handle)
{
	struct restricted_vfile *restricted = handle;
	free(restricted);
}

static struct vfile_functions restricted_io_functions = {
	restricted_vfread,
	restricted_fwrite,
	restricted_vfseek,
	restricted_vftell,
	restricted_vftruncate,
	restricted_vfclose,
	restricted_vfsync,
};

VFILE *vfrestrict(VFILE *inner, long start, long end, int ro)
{
	struct restricted_vfile *restricted;
	assert(vfseek(inner, start, SEEK_SET) == 0);
	restricted = checked_calloc(1, sizeof(struct restricted_vfile));
	restricted->inner = inner;
	restricted->start = start;
	restricted->end = end;
	restricted->ro = ro;
	return vfopen(restricted, &restricted_io_functions);
}

int vfcopy(VFILE *from, VFILE *to)
{
	uint8_t buf[256];
	size_t nbytes;

	for (;;) {
		nbytes = vfread(buf, 1, sizeof(buf), from);
		if (nbytes == 0) {
			return 0;
		}
		if (vfwrite(buf, 1, nbytes, to) != nbytes) {
			return -1;
		}
	}
}

