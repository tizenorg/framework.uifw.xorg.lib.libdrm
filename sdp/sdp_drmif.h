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

#ifndef _SDP_DRMIF_H_
#define _SDP_DRMIF_H_

#include <xf86drm.h>
#include <stdint.h>
#include "sdp_drm.h"

struct sdp_device {
	int fd;
};

/*
 * sdp Buffer Object structure
 *
 * @dev: sdp device object allocated
 * @handle: a gem handle to gem object created
 * @flags: indicate memory allocation and cache attribute types
 * @fd: file descriptor exported into dmabuf
 * @size: size to the buffer created
 * @vaddr: user space address to a gem buffer mmaped
 * @name: a gem global handle from flink request
 */
struct sdp_bo {
	struct sdp_device	*dev;
	uint32_t		handle;
	uint32_t		flags;
	int			fd;
	size_t			size;
	void			*vaddr;
	uint32_t		name;
};

/*
 * device related functions
 */
struct sdp_device *sdp_device_create(int fd);
void sdp_device_destroy(struct sdp_device *dev);

/*
 * buffer-object related functions
 */
struct sdp_bo *sdp_bo_create(struct sdp_device *dev, size_t size,
			     uint32_t flags);
struct sdp_bo *sdp_dumb_bo_create(struct sdp_device *dev,
				  const unsigned width, const unsigned height);
int sdp_dumb_bo_map(struct sdp_bo *bo, void **out);
void sdp_bo_destroy(struct sdp_bo *bo);
struct sdp_bo *sdp_bo_from_name(struct sdp_device *dev, uint32_t name);
int sdp_bo_get_name(struct sdp_bo *bo, uint32_t *name);
uint32_t sdp_bo_handle(struct sdp_bo *bo);
void *sdp_bo_map(struct sdp_bo *bo);
int sdp_prime_handle_to_fd(struct sdp_device *dev, uint32_t handle, int *fd);
int sdp_prime_fd_to_handle(struct sdp_device *dev, int fd, uint32_t *handle);

#endif /* _SDP_DRMIF_H_ */
