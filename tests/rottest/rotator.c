/*
 * DRM based rotator test program
 * Copyright 2012 Samsung Electronics
 *   YoungJun Cho <yj44.cho@samsung.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "exynos_drm.h"
#include "rottest.h"
#include "gem.h"
#include "util.h"

#include "drm_fourcc.h"

static int exynos_drm_ipp_property(int fd,
				struct drm_exynos_ipp_property *property,
				struct drm_exynos_pos *pos,
				struct drm_exynos_sz *sz)
{
	int ret = 0;

	memset(property, 0x00, sizeof(struct drm_exynos_ipp_property));

	property->config[EXYNOS_DRM_OPS_SRC].ops_id = EXYNOS_DRM_OPS_SRC;
	property->config[EXYNOS_DRM_OPS_SRC].flip = EXYNOS_DRM_FLIP_NONE;
	property->config[EXYNOS_DRM_OPS_SRC].degree = EXYNOS_DRM_DEGREE_0;
	property->config[EXYNOS_DRM_OPS_SRC].fmt = DRM_FORMAT_XRGB8888;
	property->config[EXYNOS_DRM_OPS_SRC].pos = *pos;
	property->config[EXYNOS_DRM_OPS_SRC].sz = *sz;

	property->config[EXYNOS_DRM_OPS_DST].ops_id = EXYNOS_DRM_OPS_DST;
	property->config[EXYNOS_DRM_OPS_DST].flip = EXYNOS_DRM_FLIP_NONE;
	property->config[EXYNOS_DRM_OPS_DST].degree = EXYNOS_DRM_DEGREE_90;
	property->config[EXYNOS_DRM_OPS_DST].fmt = DRM_FORMAT_XRGB8888;
	property->config[EXYNOS_DRM_OPS_DST].pos = *pos;
	property->config[EXYNOS_DRM_OPS_DST].sz = *sz;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_IPP_PROPERTY, property);
	if (ret)
		fprintf(stderr,
			"failed to DRM_IOCTL_EXYNOS_IPP_PROPERTY : %s\n",
			strerror(errno));

	return ret;
}

static int exynos_drm_ipp_buf(int fd, struct drm_exynos_ipp_buf *buf,
					enum drm_exynos_ops_id ops_id,
					enum drm_exynos_ipp_buf_ctrl ctrl,
					unsigned int gem_handle)
{
	int ret = 0;

	memset(buf, 0x00, sizeof(struct drm_exynos_ipp_buf));

	buf->ops_id = ops_id;
	buf->buf_ctrl = ctrl;
	buf->user_data = 0;
	buf->id = 0;
	buf->handle[EXYNOS_DRM_PLANAR_Y] = gem_handle;
	buf->handle[EXYNOS_DRM_PLANAR_CB] = 0;
	buf->handle[EXYNOS_DRM_PLANAR_CR] = 0;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_IPP_BUF, buf);
	if (ret)
		fprintf(stderr,
		"failed to DRM_IOCTL_EXYNOS_IPP_BUF[id:%d][ctrl:%d] : %s\n",
		ops_id, ctrl, strerror(errno));
 
	return ret;
}

static int exynos_drm_ipp_ctrl(int fd, struct drm_exynos_ipp_ctrl *ctrl,
				enum drm_exynos_ipp_cmd cmd, unsigned int use)
{
	int ret = 0;

	memset(ctrl, 0x00, sizeof(struct drm_exynos_ipp_ctrl));

	ctrl->cmd = cmd;
	ctrl->use = use;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_IPP_CTRL, ctrl);
	if (ret)
		fprintf(stderr,
		"failed to DRM_IOCTL_EXYNOS_IPP_CTRL[cmd:%d][use:%d] : %s\n",
		cmd, use, strerror(errno));

	return ret;
}

void rotator_set_mode(struct connector *c, int count, int page_flip,
								long int *usec)
{
	struct drm_exynos_pos def_pos = {0, 0, 720, 1280};
	struct drm_exynos_sz def_sz = {720, 1280};
	struct drm_exynos_ipp_property property;
	struct drm_exynos_ipp_buf buf1, buf2;
	struct drm_exynos_ipp_ctrl ctrl;
	unsigned int width, height, stride;
	int ret, i, j, x;
	struct drm_exynos_gem_create gem1, gem2;
	struct drm_exynos_gem_mmap mmap1, mmap2;
	void *usr_addr1, *usr_addr2;
	struct timeval begin, end;
	struct drm_gem_close args;
	char filename[100];

	/* For property */
	ret = exynos_drm_ipp_property(fd, &property, &def_pos, &def_sz);
	if (ret) {
		fprintf(stderr, "failed to ipp property\n");
		return;
	}

	/* For mode */
	width = height = 0;
	for (i = 0; i < count; i++) {
		connector_find_mode(&c[i]);
		if (c[i].mode == NULL) continue;
		width += c[i].mode->hdisplay;
		if (height < c[i].mode->vdisplay) height = c[i].mode->vdisplay;
	}
	stride = width * 4;

	/* For source buffer */
	ret = util_gem_create_mmap(fd, &gem1, &mmap1, stride * height);
	if (ret) {
		fprintf(stderr, "failed to gem create mmap: %s\n",
							strerror(errno));
		if (ret == -1) return;
		else if (ret == -2) goto err_gem_mmap1;
	}
	usr_addr1 = (void *)(unsigned long)mmap1.mapped;

	util_draw_buffer(usr_addr1, 1, width, height, stride, 0);

	sprintf(filename, "/opt/media/rot_src.bmp", j);
	util_write_bmp(filename, usr_addr1, width, height);

	/* For destination buffer */
	ret = util_gem_create_mmap(fd, &gem2, &mmap2, stride * height);
	if (ret) {
		fprintf(stderr, "failed to gem create mmap: %s\n",
							strerror(errno));
		if (ret == -1) goto err_gem_create2;
		else if (ret == -2) goto err_gem_mmap2;
	}
	usr_addr2 = (void*)(unsigned long)mmap2.mapped;

	util_draw_buffer(usr_addr2, 0, 0, 0, 0, mmap2.size);

	sprintf(filename, "/opt/media/rot_dst.bmp", j);
	util_write_bmp(filename, usr_addr2, height, width);

	/* For source buffer map to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf1, EXYNOS_DRM_OPS_SRC,
					IPP_BUF_CTRL_MAP, gem1.handle);
	if (ret) {
		fprintf(stderr, "failed to ipp buf src map\n");
		goto err_ipp_buf_map1;
	}

	/* For destination buffer map to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST,
					IPP_BUF_CTRL_MAP, gem2.handle);
	if (ret) {
		fprintf(stderr, "failed to ipp buf dst map\n");
		goto err_ipp_buf_map2;
	}

	for (j = 0; j < MAX_LOOP; j++) {
		/* Start */
		gettimeofday(&begin, NULL);
		ret = exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_M2M, 1);
		if (ret) {
			fprintf(stderr,
				"failed to ipp ctrl IPP_CMD_M2M start\n");
			goto err_ipp_ctrl_start;
		}

		while (1) {
			struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
			fd_set fds;

			FD_ZERO(&fds);
			FD_SET(0, &fds);
			FD_SET(fd, &fds);
			ret = select(fd + 1, &fds, NULL, NULL, &timeout);
			if (ret <= 0) {
				fprintf(stderr, "select timed out or error.\n");
				continue;
			} else if (FD_ISSET(0, &fds)) {
				break;
			}

			gettimeofday(&end, NULL);
			usec[j] = (end.tv_sec - begin.tv_sec) * 1000000 +
						(end.tv_usec - begin.tv_usec);

			sprintf(filename, "/opt/media/rot_%d.bmp", j);
			util_write_bmp(filename, usr_addr2, height, width);

			break;
		}

		/* For destination buffer queue to IPP */
		ret = exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST,
					IPP_BUF_CTRL_QUEUE, gem2.handle);
		if (ret) {
			fprintf(stderr, "failed to ipp buf dst queue\n");
			goto err_ipp_ctrl_start;
		}
	}

	/* For source buffer unmap to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf1, EXYNOS_DRM_OPS_SRC,
					IPP_BUF_CTRL_UNMAP, gem1.handle);
	if (ret) {
		fprintf(stderr, "failed to ipp buf src unmap\n");
		goto err_ipp_buf_unmap;
	}

	/* For destination buffer unmap to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST,
					IPP_BUF_CTRL_UNMAP, gem2.handle);
	if (ret < 0) {
		fprintf(stderr, "failed to ipp buf dst unmap\n");
		goto err_ipp_buf_unmap;
	}

	/* Stop */
	ret = exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_M2M, 0);
	if (ret) {
		fprintf(stderr, "failed to ipp ctrl IPP_CMD_M2M stop\n");
		goto err_ipp_buf_unmap;
	}

	munmap(usr_addr2, mmap2.size);
	munmap(usr_addr1, mmap1.size);

	memset(&args, 0x00, sizeof(struct drm_gem_close));
	args.handle = gem2.handle;
	exynos_gem_close(fd, &args);
	memset(&args, 0x00, sizeof(struct drm_gem_close));
	args.handle = gem1.handle;
	exynos_gem_close(fd, &args);

	return;

err_ipp_buf_unmap:
	exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_M2M, 0);
err_ipp_ctrl_start:
	exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST, IPP_BUF_CTRL_UNMAP,
								gem2.handle);
err_ipp_buf_map2:
	exynos_drm_ipp_buf(fd, &buf1, EXYNOS_DRM_OPS_SRC, IPP_BUF_CTRL_UNMAP,
								gem1.handle);
err_ipp_buf_map1:
	munmap(usr_addr2, mmap2.size);
err_gem_mmap2:
	memset(&args, 0x00, sizeof(struct drm_gem_close));
	args.handle = gem2.handle;
	exynos_gem_close(fd, &args);
err_gem_create2:
	munmap(usr_addr1, mmap1.size);
err_gem_mmap1:
	memset(&args, 0x00, sizeof(struct drm_gem_close));
	args.handle = gem1.handle;
	exynos_gem_close(fd, &args);
}
