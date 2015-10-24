/*
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/mman.h>
#include <linux/stddef.h>

#include <xf86drm.h>

#include "sdp_drm.h"
#include "sdp_drmif.h"

#ifndef MAX_LINE_SIZE
#define MAX_LINE_SIZE 256
#endif

/*
 * Create sdp drm device object
 *
 * @fd: file descriptor to sdp drm driver opened
 *
 * If true, return the device object else NULL
 */
struct sdp_device *sdp_device_create(int fd)
{
	struct sdp_device *dev;
        char strErroBuf[MAX_LINE_SIZE] = {0,};

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		fprintf(stderr, "failed to create device[%s]\n",
				strerror_r(errno,strErroBuf, MAX_LINE_SIZE));
		return NULL;
	}

	dev->fd = fd;

	return dev;
}

/*
 * Destroy sdp drm device object
 *
 * @dev: sdp drm device object
 */
void sdp_device_destroy(struct sdp_device *dev)
{
	free(dev);
}

/*
 * Create a sdp buffer object to sdp drm device
 *
 * @dev: sdp drm device object
 * @size: user-desired size
 * @flags: user-desired memory type
 *
 * User can set one type among two types to memory allocation. The default type
 * is contiguous memory. If you set SDP_DRM_GEM_NONCONTIG to flags, it will
 * be allocated noncontiguous memory
 *
 * If true, return a sdp buffer object else NULL
 */
struct sdp_bo *sdp_bo_create(struct sdp_device *dev, size_t size,
			     uint32_t flags)
{
	struct sdp_bo *bo;
	struct sdp_drm_gem_create arg;
	char strErroBuf[MAX_LINE_SIZE] = {0,};

	if (!size) {
		fprintf(stderr, "invalid size\n");
		return NULL;
	}

	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		fprintf(stderr, "failed to create bo[%s]\n", strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
		return NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.size = size;
	arg.flags = flags;

	if (drmIoctl(dev->fd, DRM_IOCTL_SDP_GEM_CREATE, &arg)){
		fprintf(stderr, "failed to create gem object[%s]\n",
				strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
		goto err_free;
	}

	bo->dev = dev;
	bo->handle = arg.handle;
	bo->size = size;
	bo->flags = flags;

	return bo;

err_free:
	free(bo);
	return NULL;
}

/* from libkms/dumb.c */
struct sdp_bo *sdp_dumb_bo_create(struct sdp_device *dev,
				  const unsigned width, const unsigned height)
{
	struct drm_mode_create_dumb arg;
	struct sdp_bo *bo;
	int ret;
	char strErroBuf[MAX_LINE_SIZE] = {0,};
	
	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		fprintf(stderr, "failed to create bo[%s]\n", strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
		return NULL;
	}

	memset(&arg, 0, sizeof(arg));

	/* All BO_TYPE currently are 32bpp formats */
	arg.bpp = 32;
	arg.width = width;
	arg.height = height;

	ret = drmIoctl(dev->fd, DRM_IOCTL_MODE_CREATE_DUMB, &arg);
	if (ret)
		goto err_free;

	bo->dev = dev;
	bo->handle = arg.handle;
	bo->size = arg.size;

	return bo;

err_free:
	free(bo);
	return NULL;
}

/* from libkms/dumb.c */
int sdp_dumb_bo_map(struct sdp_bo *bo, void **out)
{
	struct drm_mode_map_dumb arg;
	void *map = NULL;
	int ret;

	memset(&arg, 0, sizeof(arg));
	arg.handle = bo->handle;

	ret = drmIoctl(bo->dev->fd, DRM_IOCTL_MODE_MAP_DUMB, &arg);
	if (ret)
		return ret;

	map = mmap(0, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, bo->dev->fd,
			arg.offset);
	if (map == MAP_FAILED)
		return -errno;

	bo->vaddr = map;
	*out = bo->vaddr;

	return 0;
}

/*
 * Destroy a sdp buffer object
 *
 * @bo: a sdp buffer object to be destroyed
 */
void sdp_bo_destroy(struct sdp_bo *bo)
{
	if (!bo)
		return;

	if (bo->vaddr)
		munmap(bo->vaddr, bo->size);

	if (bo->handle) {
		struct drm_gem_close arg = {
			.handle = bo->handle,
		};

		drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_CLOSE, &arg);
	}

	free(bo);
}

/*
 * Get a sdp buffer object from a gem global object name
 *
 * @dev: a sdp device object
 * @name: a gem global object name exported by another process
 *
 * This interface is used to get a sdp buffer object from a gem
 * global object name sent by another process for buffer sharing
 *
 * If true, return a sdp buffer object else NULL
 */
struct sdp_bo *sdp_bo_from_name(struct sdp_device *dev, uint32_t name)
{
	struct sdp_bo *bo;
	struct drm_gem_open arg;
	char strErroBuf[MAX_LINE_SIZE] = {0,};

	bo = calloc(1, sizeof(*bo));
	if (!bo) {
		fprintf(stderr, "failed to allocate bo[%s]\n", strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
		return NULL;
	}

	memset(&arg, 0, sizeof(arg));
	arg.name = name;

	if (drmIoctl(dev->fd, DRM_IOCTL_GEM_OPEN, &arg)) {
		fprintf(stderr, "failed to open gem object[%s]\n",
				strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
		goto err_free;
	}

	bo->dev = dev;
	bo->name = name;
	bo->handle = arg.handle;

	return bo;

err_free:
	free(bo);
	return NULL;
}

/*
 * Get a gem global object name from a gem object handle
 *
 * @bo: a sdp buffer object including gem handle
 * @name: a gem global object name to be got by kernel driver
 *
 * This interface is used to get a gem global object name from a gem object
 * handle to a buffer that wants to share it with another process
 *
 * If true, return 0 else negative
 */
int sdp_bo_get_name(struct sdp_bo *bo, uint32_t *name)
{
	if (!bo->name) {
		struct drm_gem_flink arg;
		int ret;
                char strErroBuf[MAX_LINE_SIZE] = {0,};
	
		memset(&arg, 0, sizeof(arg));
		arg.handle = bo->handle;

		ret = drmIoctl(bo->dev->fd, DRM_IOCTL_GEM_FLINK, &arg);
		if (ret) {
			fprintf(stderr, "failed to get gem global name[%s]\n", strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
			return ret;
		}

		bo->name = arg.name;
	}

	*name = bo->name;

	return 0;
}

uint32_t sdp_bo_handle(struct sdp_bo *bo)
{
	return bo->handle;
}

/*
 * Export gem object to dmabuf as file descriptor
 *
 * @dev: a sdp device object
 * @handle: gem handle to be exported into dmabuf as file descriptor
 * @fd: file descriptor to dmabuf exported from gem handle and
 *	returned by kernel side
 *
 * If true, return 0 else negative
 */
int sdp_prime_handle_to_fd(struct sdp_device *dev, uint32_t handle, int *fd)
{
	int ret;
	char strErroBuf[MAX_LINE_SIZE] = {0,};

	ret = drmPrimeHandleToFD(dev->fd, handle, DRM_CLOEXEC, fd);
	if (ret) {
		fprintf(stderr, "failed to HandleToFD[%s]\n", strerror_r(errno, strErroBuf, MAX_LINE_SIZE));
		return ret;
	}

	return 0;
}

/*
 * Import file descriptor into gem handle
 *
 * @dev: a sdp device object
 * @fd: file descriptor exported into dmabuf
 * @handle: gem handle to gem object imported from file descriptor
 *	and returned by kernel side
 *
 * If true, return 0 else negative
 */
int sdp_prime_fd_to_handle(struct sdp_device *dev, int fd, uint32_t *handle)
{
	int ret;
	char strErroBuf[MAX_LINE_SIZE] = {0,};
	ret = drmPrimeFDToHandle(dev->fd, fd, handle);
	if (ret) {
		fprintf(stderr, "failed to FDToHandle[%s]\n", strerror_r(errno,strErroBuf, MAX_LINE_SIZE));
		return ret;
	}

	return 0;
}
