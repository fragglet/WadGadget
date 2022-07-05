
#include <stdlib.h>
#include <assert.h>

#include "vfile.h"

struct _VFILE {
	const struct vfile_functions *functions;
	void *handle;
};

VFILE *vfopen(void *handle, struct vfile_functions *funcs)
{
	VFILE *result;
	result = calloc(1, sizeof(VFILE));
	assert(result != NULL);
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

int vfseek(VFILE *stream, long offset)
{
	return stream->functions->seek(stream->handle, offset);
}

long vftell(VFILE *stream)
{
	return stream->functions->tell(stream->handle);
}

void vfclose(VFILE *stream)
{
	stream->functions->close(stream->handle);
	free(stream);
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

static int wrapped_fseek(void *handle, long offset)
{
	return fseek(handle, offset, SEEK_SET);
}

static long wrapped_ftell(void *handle)
{
	return ftell(handle);
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
	wrapped_fclose,
};

VFILE *vfwrapfile(FILE *stream)
{
	return vfopen(stream, &wrapped_io_functions);
}

