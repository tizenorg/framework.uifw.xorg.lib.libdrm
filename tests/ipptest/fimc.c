/*
 * DRM based fimc test program
 * Copyright 2012 Samsung Electronics
 *   Eunchul Kim <chulspro.kim@sasmsung.com>
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
#include "fimctest.h"
#include "gem.h"
#include "util.h"

#include "drm_fourcc.h"

static int exynos_drm_ipp_property(int fd,
				struct drm_exynos_ipp_property *property,
				struct drm_exynos_sz *def_sz,
				enum drm_exynos_ipp_cmd cmd,
				enum drm_exynos_degree degree)
{
	struct drm_exynos_pos crop_pos = {0, 0, def_sz->hsize, def_sz->vsize};
	struct drm_exynos_pos scale_pos = {0, 0, def_sz->hsize, def_sz->vsize};
	struct drm_exynos_sz src_sz = {def_sz->hsize, def_sz->vsize};
	struct drm_exynos_sz dst_sz = {def_sz->hsize, def_sz->vsize};
	int ret = 0;

	memset(property, 0x00, sizeof(struct drm_exynos_ipp_property));

	switch(cmd) {
	case IPP_CMD_M2M:
		property->config[EXYNOS_DRM_OPS_SRC].ops_id = EXYNOS_DRM_OPS_SRC;
		property->config[EXYNOS_DRM_OPS_SRC].flip = EXYNOS_DRM_FLIP_NONE;
		property->config[EXYNOS_DRM_OPS_SRC].degree = EXYNOS_DRM_DEGREE_0;
		property->config[EXYNOS_DRM_OPS_SRC].fmt = DRM_FORMAT_XRGB8888;
		property->config[EXYNOS_DRM_OPS_SRC].pos = crop_pos;
		property->config[EXYNOS_DRM_OPS_SRC].sz = src_sz;

		property->config[EXYNOS_DRM_OPS_DST].ops_id = EXYNOS_DRM_OPS_DST;
		property->config[EXYNOS_DRM_OPS_DST].flip = EXYNOS_DRM_FLIP_NONE;
		property->config[EXYNOS_DRM_OPS_DST].degree = degree;
		property->config[EXYNOS_DRM_OPS_DST].fmt = DRM_FORMAT_XRGB8888;
		if (property->config[EXYNOS_DRM_OPS_DST].degree == EXYNOS_DRM_DEGREE_90) {
			dst_sz.hsize = def_sz->vsize;
			dst_sz.vsize = def_sz->hsize;

			scale_pos.w = def_sz->vsize;
			scale_pos.h = def_sz->hsize;
		}
		property->config[EXYNOS_DRM_OPS_DST].pos = scale_pos;
		property->config[EXYNOS_DRM_OPS_DST].sz = dst_sz;
		break;
	case IPP_CMD_WB:
		property->config[EXYNOS_DRM_OPS_SRC].ops_id = EXYNOS_DRM_OPS_SRC;
		property->config[EXYNOS_DRM_OPS_SRC].flip = EXYNOS_DRM_FLIP_NONE;
		property->config[EXYNOS_DRM_OPS_SRC].degree = EXYNOS_DRM_DEGREE_0;
		property->config[EXYNOS_DRM_OPS_SRC].fmt = DRM_FORMAT_YUV444;
		property->config[EXYNOS_DRM_OPS_SRC].pos = crop_pos;
		property->config[EXYNOS_DRM_OPS_SRC].sz = src_sz;

		property->config[EXYNOS_DRM_OPS_DST].ops_id = EXYNOS_DRM_OPS_DST;
		property->config[EXYNOS_DRM_OPS_DST].flip = EXYNOS_DRM_FLIP_NONE;
		property->config[EXYNOS_DRM_OPS_DST].degree = degree;
		property->config[EXYNOS_DRM_OPS_DST].fmt = DRM_FORMAT_XRGB8888;
		if (property->config[EXYNOS_DRM_OPS_DST].degree == EXYNOS_DRM_DEGREE_90) {
			dst_sz.hsize = def_sz->vsize;
			dst_sz.vsize = def_sz->hsize;

			scale_pos.w = def_sz->vsize;
			scale_pos.h = def_sz->hsize;
		}
		property->config[EXYNOS_DRM_OPS_DST].pos = scale_pos;
		property->config[EXYNOS_DRM_OPS_DST].sz = dst_sz;
		break;
	case IPP_CMD_OUT:
	default:
		ret = -EINVAL;
		return ret;
	}

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
					int id,
					unsigned int gem_handle)
{
	int ret = 0;

	memset(buf, 0x00, sizeof(struct drm_exynos_ipp_buf));

	buf->ops_id = ops_id;
	buf->buf_ctrl = ctrl;
	buf->user_data = 0;
	buf->id = id;
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

void fimc_m2m_set_mode(struct connector *c, int count, int page_flip,
								long int *usec)
{
	struct drm_exynos_ipp_property property;
	struct drm_exynos_ipp_ctrl ctrl;
	struct drm_exynos_sz def_sz = {720, 1280};
	struct drm_exynos_ipp_buf buf1, buf2;
	unsigned int width=720, height=1280, stride;
	int ret, i, j, x;
	struct drm_exynos_gem_create gem1, gem2;
	struct drm_exynos_gem_mmap mmap1, mmap2;
	void *usr_addr1, *usr_addr2;
	struct timeval begin, end;
	struct drm_gem_close args;
	char filename[100];

	/* For property */
	ret = exynos_drm_ipp_property(fd, &property, &def_sz, IPP_CMD_M2M, EXYNOS_DRM_DEGREE_90);
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

	sprintf(filename, "/opt/media/fimc_m2m_src.bmp", j);
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

	sprintf(filename, "/opt/media/fimc_m2m_dst.bmp", j);
	util_write_bmp(filename, usr_addr2, height, width);

	/* For source buffer map to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf1, EXYNOS_DRM_OPS_SRC,
					IPP_BUF_CTRL_MAP, 0, gem1.handle);
	if (ret) {
		fprintf(stderr, "failed to ipp buf src map\n");
		goto err_ipp_buf_map1;
	}

	/* For destination buffer map to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST,
					IPP_BUF_CTRL_MAP, 0, gem2.handle);
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

			sprintf(filename, "/opt/media/fimc_m2m_%d.bmp", j);
			util_write_bmp(filename, usr_addr2, height, width);

			break;
		}

		/* For destination buffer queue to IPP */
		ret = exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST,
					IPP_BUF_CTRL_QUEUE, 0, gem2.handle);
		if (ret) {
			fprintf(stderr, "failed to ipp buf dst queue\n");
			goto err_ipp_ctrl_start;
		}
	}

	/* For source buffer unmap to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf1, EXYNOS_DRM_OPS_SRC,
					IPP_BUF_CTRL_UNMAP, 0, gem1.handle);
	if (ret) {
		fprintf(stderr, "failed to ipp buf src unmap\n");
		goto err_ipp_buf_unmap;
	}

	/* For destination buffer unmap to IPP */
	ret = exynos_drm_ipp_buf(fd, &buf2, EXYNOS_DRM_OPS_DST,
					IPP_BUF_CTRL_UNMAP, 0, gem2.handle);
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
								0, gem2.handle);
err_ipp_buf_map2:
	exynos_drm_ipp_buf(fd, &buf1, EXYNOS_DRM_OPS_SRC, IPP_BUF_CTRL_UNMAP,
								0, gem1.handle);
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

int fimc_event_handler(struct drm_exynos_ipp_buf *buf, struct drm_exynos_gem_create *gem,
	void **usr_addr, unsigned int width, unsigned int height)
{
	char buffer[1024];
	int len, i;
	struct drm_event *e;
	struct drm_exynos_ipp_event *ipp_event;
	char filename[100];
	int ret = 0;
	static bmp_idx = 0;

	len = read(fd, buffer, sizeof buffer);
	if (len == 0)
		return 0;
	if (len < sizeof *e)
		return -1;

	i = 0;
	while (i < len) {
		e = (struct drm_event *) &buffer[i];
		switch (e->type) {
		case DRM_EXYNOS_IPP_EVENT:
			ipp_event = (struct drm_exynos_ipp_event *) e;

			fprintf(stderr, "%s:buf_idx[%d]bmp_idx[%d]\n", __func__, ipp_event->buf_idx, bmp_idx++);
			sprintf(filename, "/opt/media/fimc_wb_%d.bmp", bmp_idx);
			util_write_bmp(filename, usr_addr[ipp_event->buf_idx], width, height);

			/* For destination buffer queue to IPP */
			ret = exynos_drm_ipp_buf(fd, &buf[ipp_event->buf_idx], EXYNOS_DRM_OPS_DST,
						IPP_BUF_CTRL_QUEUE, ipp_event->buf_idx, gem[ipp_event->buf_idx].handle);
			if (ret) {
				fprintf(stderr, "failed to ipp buf dst queue\n");
				goto err_ipp_ctrl_close;
			}
			break;
		default:
			break;
		}
		i += e->length;
	}

err_ipp_ctrl_close:
	return ret;
}

void fimc_wb_set_mode(struct connector *c, int count, int page_flip,
								long int *usec)
{
	struct drm_exynos_pos def_pos = {0, 0, 720, 1280};
	struct drm_exynos_sz def_sz = {720, 1280};
	struct drm_exynos_ipp_property property;
	struct drm_exynos_gem_create gem[MAX_BUF];
	struct drm_exynos_gem_mmap mmap[MAX_BUF];
	struct drm_exynos_ipp_buf buf[MAX_BUF];	
	void *usr_addr[MAX_BUF];
	struct drm_exynos_ipp_ctrl ctrl;
	unsigned int width, height, stride;
	int ret, i, j;
	struct timeval begin, end;
	struct drm_gem_close args;

	/* For property */
	ret = exynos_drm_ipp_property(fd, &property, &def_sz, IPP_CMD_WB, EXYNOS_DRM_DEGREE_0);
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

	/* For destination buffer */
	for (i = 0; i < MAX_BUF; i++) {
		ret = util_gem_create_mmap(fd, &gem[i], &mmap[i], stride * height);
		if (ret) {
			fprintf(stderr, "failed to gem create mmap: %s\n",
								strerror(errno));
			if (ret == -1) return;
			else if (ret == -2) goto err_ipp_ctrl_close;
		}
		usr_addr[i] = (void *)(unsigned long)mmap[i].mapped;
		/* For destination buffer map to IPP */
		ret = exynos_drm_ipp_buf(fd, &buf[i], EXYNOS_DRM_OPS_DST,
						IPP_BUF_CTRL_MAP, i, gem[i].handle);
		if (ret) {
			fprintf(stderr, "failed to ipp buf dst map\n");
			goto err_ipp_ctrl_close;
		}
	}


	/* Start */
	gettimeofday(&begin, NULL);
	ret = exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_WB, 1);
	if (ret) {
		fprintf(stderr,
			"failed to ipp ctrl IPP_CMD_WB start\n");
		goto err_ipp_ctrl_close;
	}

	j = 0;
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
			fprintf(stderr, "select error.\n");
			break;
		}

		gettimeofday(&end, NULL);
		usec[j] = (end.tv_sec - begin.tv_sec) * 1000000 +
					(end.tv_usec - begin.tv_usec);

		if (property.config[EXYNOS_DRM_OPS_DST].degree == EXYNOS_DRM_DEGREE_90 ||
			property.config[EXYNOS_DRM_OPS_DST].degree == EXYNOS_DRM_DEGREE_270) {
			if(fimc_event_handler(buf, gem, usr_addr, height, width) < 0)
				break;
		} else {
			if(fimc_event_handler(buf, gem, usr_addr, width, height) < 0)
				break;
		}

		if (++j > MAX_LOOP)
			break;

		if (j == HALF_LOOP) {
			/* Stop */
			ret = exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_WB, 0);
			if (ret) {
				fprintf(stderr, "failed to ipp ctrl IPP_CMD_WB stop\n");
				goto err_ipp_ctrl_close;
			}

			/* For property */
			ret = exynos_drm_ipp_property(fd, &property, &def_sz, IPP_CMD_WB, EXYNOS_DRM_DEGREE_90);
			if (ret) {
				fprintf(stderr, "failed to ipp property\n");
				goto err_ipp_ctrl_close;
			}

			/* Start */
			ret = exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_WB, 1);
			if (ret) {
				fprintf(stderr,
					"failed to ipp ctrl IPP_CMD_WB start\n");
				goto err_ipp_ctrl_close;
			}
		}

		gettimeofday(&begin, NULL);
	}

err_ipp_ctrl_close:
	/* For destination buffer unmap to IPP */
	for (i = 0; i < MAX_BUF; i++) {
		ret = exynos_drm_ipp_buf(fd, &buf[i], EXYNOS_DRM_OPS_DST,
						IPP_BUF_CTRL_UNMAP, i, gem[i].handle);
		if (ret < 0)
			fprintf(stderr, "failed to ipp buf dst unmap\n");
	}

	/* Stop */
	ret = exynos_drm_ipp_ctrl(fd, &ctrl, IPP_CMD_WB, 0);
	if (ret)
		fprintf(stderr, "failed to ipp ctrl IPP_CMD_WB stop\n");

	for (i = 0; i < MAX_BUF; i++) {
		munmap(usr_addr[i], mmap[i].size);
		memset(&args, 0x00, sizeof(struct drm_gem_close));
		args.handle = gem[i].handle;
		exynos_gem_close(fd, &args);
	}

	return;
}

void fimc_output_set_mode(struct connector *c, int count, int page_flip,
								long int *usec)
{
	fprintf(stderr, "not supported. please wait v2\n");
}

