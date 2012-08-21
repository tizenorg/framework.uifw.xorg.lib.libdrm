/*
 * DRM based mode setting test program
 * Copyright 2008 Tungsten Graphics
 *   Jakob Bornecrantz <jakob@tungstengraphics.com>
 * Copyright 2008 Intel Corporation
 *   Jesse Barnes <jesse.barnes@intel.com>
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

/*
 * This fairly simple test program dumps output in a similar format to the
 * "xrandr" tool everyone knows & loves.  It's necessarily slightly different
 * since the kernel separates outputs into encoder and connector structures,
 * each with their own unique ID.  The program also allows test testing of the
 * memory management and mode setting APIs by allowing the user to specify a
 * connector and mode to use for mode setting.  If all works as expected, a
 * blue background should be painted on the monitor attached to the specified
 * connector after the selected mode is set.
 *
 * TODO: use cairo to write the mode info on the selected output once
 *       the mode has been programmed, along with possible test patterns.
 */
#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "xf86drm.h"
#include "xf86drmMode.h"
#include "libkms.h"

#include "exynos_drm.h"
#include "exynos_drmif.h"

#include "ump_ref_drv.h"

#ifdef HAVE_CAIRO
#include <math.h>
#include <cairo.h>
#endif

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) + 1))
#define DEFAULT_SIZE	(1280 * 720 * 4)
#define MAX_CMD		5

/*
 * DIRECT_MAP:
 *	- at mapping request, physical memory allocated by gem interface
 *	would be mmaped to user space directly.
 *
 * INDIRECT_MAP:
 *	- when user tries to access user space memory after mmap,
 *	in kernel side, the page fault occurs and then the memory would
 *	be mmaped to physical memory. user can not aware of this moment.
 *
 *	P.S. in this feature, mmap call is not real-mmaped, just fake-mmaped
 *	and to test it, enable only one of them please.
 */

#define DIRECT_MAP
//#define INDIRECT_MAP

#if 0
#define UMP_TEST

#define GET_PHY_TEST

#define VIDI_TEST

#define GEMGET_TEST
#endif

enum exynos_test_type {
	CMD_ALLOC		= 1 << 0,
	CMD_CACHE		= 1 << 1,
	CMD_PRIME		= 1 << 2,
	CMD_UMP			= 1 << 3,

	ALLOC_CONTIG		= 1 << 4,
	ALLOC_NONCONTIG		= 1 << 5,
	ALLOC_USERPTR		= 1 << 6,

	ALLOC_MAP_C		= 1 << 7,
	ALLOC_MAP_NC		= 1 << 8,
	ALLOC_MAP_WC		= 1 << 9,

	CACHE_L1		= 1 << 10,
	CACHE_L2		= 1 << 11,
	CACHE_ALL_CORES		= 1 << 12,
	CACHE_ALL_CACHES	= CACHE_L1 | CACHE_L2,
	CACHE_ALL_CACHES_CORES	= CACHE_ALL_CACHES | CACHE_ALL_CORES,

	CACHE_CLEAN_ALL		= 0 << 13,
	CACHE_INV_ALL		= 0 << 14,
	CACHE_FLUSH_ALL		= CACHE_CLEAN_ALL | CACHE_INV_ALL,

	CACHE_CLEAN_RANGE	= 1 << 13,
	CACHE_INV_RANGE		= 1 << 14,
	CACHE_FLUSH_RANGE	= CACHE_CLEAN_RANGE | CACHE_INV_RANGE,

	DMABUF_IMPORT		= 1 << 15,
	DMABUF_EXPORT		= 1 << 16,
	DMABUF_ALL		= DMABUF_IMPORT | DMABUF_EXPORT
};

struct exynos_gem_test {
	char		*scmd[MAX_CMD];

	unsigned int	cmd_type;
	unsigned int	mem_type;
	unsigned int	map_type;
	unsigned int	cache_type;
	unsigned int	cache_op_type;
	unsigned long	start_offset;
	unsigned long	end_offset;
	unsigned int	dmabuf_type;
};

struct exynos_gem_test exynos_test;

drmModeRes *resources;
int fd, modes;

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct type_name {
	int type;
	char *name;
};

#define type_name_fn(res) \
char * res##_str(int type) {			\
	int i;						\
	for (i = 0; i < ARRAY_SIZE(res##_names); i++) { \
		if (res##_names[i].type == type)	\
			return res##_names[i].name;	\
	}						\
	return "(invalid)";				\
}

struct type_name encoder_type_names[] = {
	{ DRM_MODE_ENCODER_NONE, "none" },
	{ DRM_MODE_ENCODER_DAC, "DAC" },
	{ DRM_MODE_ENCODER_TMDS, "TMDS" },
	{ DRM_MODE_ENCODER_LVDS, "LVDS" },
	{ DRM_MODE_ENCODER_TVDAC, "TVDAC" },
};

type_name_fn(encoder_type)

struct type_name connector_status_names[] = {
	{ DRM_MODE_CONNECTED, "connected" },
	{ DRM_MODE_DISCONNECTED, "disconnected" },
	{ DRM_MODE_UNKNOWNCONNECTION, "unknown" },
};

type_name_fn(connector_status)

struct type_name connector_type_names[] = {
	{ DRM_MODE_CONNECTOR_Unknown, "unknown" },
	{ DRM_MODE_CONNECTOR_VGA, "VGA" },
	{ DRM_MODE_CONNECTOR_DVII, "DVI-I" },
	{ DRM_MODE_CONNECTOR_DVID, "DVI-D" },
	{ DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
	{ DRM_MODE_CONNECTOR_Composite, "composite" },
	{ DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
	{ DRM_MODE_CONNECTOR_LVDS, "LVDS" },
	{ DRM_MODE_CONNECTOR_Component, "component" },
	{ DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
	{ DRM_MODE_CONNECTOR_DisplayPort, "displayport" },
	{ DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
	{ DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
	{ DRM_MODE_CONNECTOR_TV, "TV" },
	{ DRM_MODE_CONNECTOR_eDP, "embedded displayport" },
	{ DRM_MODE_CONNECTOR_VIRTUAL, "Virtual" },
};

type_name_fn(connector_type)

void dump_encoders(void)
{
	drmModeEncoder *encoder;
	int i;

	printf("Encoders:\n");
	printf("id\tcrtc\ttype\tpossible crtcs\tpossible clones\t\n");
	for (i = 0; i < resources->count_encoders; i++) {
		encoder = drmModeGetEncoder(fd, resources->encoders[i]);

		if (!encoder) {
			fprintf(stderr, "could not get encoder %i: %s\n",
				resources->encoders[i], strerror(errno));
			continue;
		}
		printf("%d\t%d\t%s\t0x%08x\t0x%08x\n",
		       encoder->encoder_id,
		       encoder->crtc_id,
		       encoder_type_str(encoder->encoder_type),
		       encoder->possible_crtcs,
		       encoder->possible_clones);
		drmModeFreeEncoder(encoder);
	}
	printf("\n");
}

void dump_mode(drmModeModeInfo *mode)
{
	printf("  %s %d %d %d %d %d %d %d %d %d\n",
	       mode->name,
	       mode->vrefresh,
	       mode->hdisplay,
	       mode->hsync_start,
	       mode->hsync_end,
	       mode->htotal,
	       mode->vdisplay,
	       mode->vsync_start,
	       mode->vsync_end,
	       mode->vtotal);
}

static void
dump_props(drmModeConnector *connector)
{
	drmModePropertyPtr props;
	int i;

	for (i = 0; i < connector->count_props; i++) {
		props = drmModeGetProperty(fd, connector->props[i]);
		printf("\t%s, flags %d\n", props->name, props->flags);
		drmModeFreeProperty(props);
	}
}

void dump_connectors(void)
{
	drmModeConnector *connector;
	int i, j;

	printf("Connectors:\n");
	printf("id\tencoder\tstatus\t\ttype\tsize (mm)\tmodes\tencoders\n");
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(fd, resources->connectors[i]);

		if (!connector) {
			fprintf(stderr, "could not get connector %i: %s\n",
				resources->connectors[i], strerror(errno));
			continue;
		}

		printf("%d\t%d\t%s\t%s\t%dx%d\t\t%d\t",
		       connector->connector_id,
		       connector->encoder_id,
		       connector_status_str(connector->connection),
		       connector_type_str(connector->connector_type),
		       connector->mmWidth, connector->mmHeight,
		       connector->count_modes);

		for (j = 0; j < connector->count_encoders; j++)
			printf("%s%d", j > 0 ? ", " : "", connector->encoders[j]);
		printf("\n");

		if (!connector->count_modes)
			continue;

		printf("  modes:\n");
		printf("  name refresh (Hz) hdisp hss hse htot vdisp "
		       "vss vse vtot)\n");
		for (j = 0; j < connector->count_modes; j++)
			dump_mode(&connector->modes[j]);

		printf("  props:\n");
		dump_props(connector);

		drmModeFreeConnector(connector);
	}
	printf("\n");
}

void dump_crtcs(void)
{
	drmModeCrtc *crtc;
	int i;

	printf("CRTCs:\n");
	printf("id\tfb\tpos\tsize\n");
	for (i = 0; i < resources->count_crtcs; i++) {
		crtc = drmModeGetCrtc(fd, resources->crtcs[i]);

		if (!crtc) {
			fprintf(stderr, "could not get crtc %i: %s\n",
				resources->crtcs[i], strerror(errno));
			continue;
		}
		printf("%d\t%d\t(%d,%d)\t(%dx%d)\n",
		       crtc->crtc_id,
		       crtc->buffer_id,
		       crtc->x, crtc->y,
		       crtc->width, crtc->height);
		dump_mode(&crtc->mode);

		drmModeFreeCrtc(crtc);
	}
	printf("\n");
}

void dump_framebuffers(void)
{
	drmModeFB *fb;
	int i;

	printf("Frame buffers:\n");
	printf("id\tsize\tpitch\n");
	for (i = 0; i < resources->count_fbs; i++) {
		fb = drmModeGetFB(fd, resources->fbs[i]);

		if (!fb) {
			fprintf(stderr, "could not get fb %i: %s\n",
				resources->fbs[i], strerror(errno));
			continue;
		}
		printf("%u\t(%ux%u)\t%u\n",
		       fb->fb_id,
		       fb->width, fb->height,
		       fb->pitch);

		drmModeFreeFB(fb);
	}
	printf("\n");
}

void exynos_test_dump(void)
{

	printf("cmd_type = %d\n", exynos_test.cmd_type);
	printf("mem_type = %d\n", exynos_test.mem_type);
	printf("map_type = %d\n", exynos_test.map_type);
	printf("cache_type = %d\n", exynos_test.cache_type);
	printf("cache_type = %d\n", exynos_test.cache_op_type);
	printf("start_offset = 0x%lx\n", exynos_test.start_offset);
	printf("end_offset = 0x%lx\n", exynos_test.end_offset);
	printf("dmabuf_type = %d\n", exynos_test.dmabuf_type);
}

/*
 * Mode setting with the kernel interfaces is a bit of a chore.
 * First you have to find the connector in question and make sure the
 * requested mode is available.
 * Then you need to find the encoder attached to that connector so you
 * can bind it with a free crtc.
 */
struct connector {
	uint32_t id;
	char mode_str[64];
	drmModeModeInfo *mode;
	drmModeEncoder *encoder;
	int crtc;
	unsigned int fb_id[2], current_fb_id;
	struct timeval start;

	int swap_count;
};	

static void
connector_find_mode(struct connector *c)
{
	drmModeConnector *connector;
	int i, j;

	/* First, find the connector & mode */
	c->mode = NULL;
	for (i = 0; i < resources->count_connectors; i++) {
		connector = drmModeGetConnector(fd, resources->connectors[i]);

		if (!connector) {
			fprintf(stderr, "could not get connector %i: %s\n",
				resources->connectors[i], strerror(errno));
			drmModeFreeConnector(connector);
			continue;
		}

		if (!connector->count_modes) {
			drmModeFreeConnector(connector);
			continue;
		}

		if (connector->connector_id != c->id) {
			drmModeFreeConnector(connector);
			continue;
		}

		for (j = 0; j < connector->count_modes; j++) {
			c->mode = &connector->modes[j];
			if (!strcmp(c->mode->name, c->mode_str))
				break;
		}

		/* Found it, break out */
		if (c->mode)
			break;

		drmModeFreeConnector(connector);
	}

	if (!c->mode) {
		fprintf(stderr, "failed to find mode \"%s\"\n", c->mode_str);
		return;
	}

	/* Now get the encoder */
	for (i = 0; i < resources->count_encoders; i++) {
		c->encoder = drmModeGetEncoder(fd, resources->encoders[i]);

		if (!c->encoder) {
			fprintf(stderr, "could not get encoder %i: %s\n",
				resources->encoders[i], strerror(errno));
			drmModeFreeEncoder(c->encoder);
			continue;
		}

		if (c->encoder->encoder_id  == connector->encoder_id)
			break;

		drmModeFreeEncoder(c->encoder);
	}

	if (c->crtc == -1)
		c->crtc = c->encoder->crtc_id;
}

static struct kms_bo *
allocate_buffer(struct kms_driver *kms,
		int width, int height, int *stride)
{
	struct kms_bo *bo;
	unsigned bo_attribs[] = {
		KMS_WIDTH,   0,
		KMS_HEIGHT,  0,
		KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
		KMS_TERMINATE_PROP_LIST
	};
	int ret;

	bo_attribs[1] = width;
	bo_attribs[3] = height;

	ret = kms_bo_create(kms, bo_attribs, &bo);
	if (ret) {
		fprintf(stderr, "failed to alloc buffer: %s\n",
			strerror(-ret));
		return NULL;
	}

	ret = kms_bo_get_prop(bo, KMS_PITCH, stride);
	if (ret) {
		fprintf(stderr, "failed to retreive buffer stride: %s\n",
			strerror(-ret));
		kms_bo_destroy(&bo);
		return NULL;
	}

	return bo;
}

static void
make_pwetty(void *data, int width, int height, int stride)
{
#ifdef HAVE_CAIRO
	cairo_surface_t *surface;
	cairo_t *cr;
	int x, y;

	surface = cairo_image_surface_create_for_data(data,
						      CAIRO_FORMAT_ARGB32,
						      width, height,
						      stride);
	cr = cairo_create(surface);
	cairo_surface_destroy(surface);

	cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
	for (x = 0; x < width; x += 250)
		for (y = 0; y < height; y += 250) {
			char buf[64];

			cairo_move_to(cr, x, y - 20);
			cairo_line_to(cr, x, y + 20);
			cairo_move_to(cr, x - 20, y);
			cairo_line_to(cr, x + 20, y);
			cairo_new_sub_path(cr);
			cairo_arc(cr, x, y, 10, 0, M_PI * 2);
			cairo_set_line_width(cr, 4);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_stroke_preserve(cr);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_set_line_width(cr, 2);
			cairo_stroke(cr);

			snprintf(buf, sizeof buf, "%d, %d", x, y);
			cairo_move_to(cr, x + 20, y + 20);
			cairo_text_path(cr, buf);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_stroke_preserve(cr);
			cairo_set_source_rgb(cr, 1, 1, 1);
			cairo_fill(cr);
		}

	cairo_destroy(cr);
#endif
}

static int
create_test_buffer(struct kms_driver *kms,
		   int width, int height, int *stride_out,
		   struct kms_bo **bo_out)
{
	struct kms_bo *bo;
	int ret, i, j, stride;
	void *virtual;

	bo = allocate_buffer(kms, width, height, &stride);
	if (!bo)
		return -1;

	ret = kms_bo_map(bo, &virtual);
	if (ret) {
		fprintf(stderr, "failed to map buffer: %s\n",
			strerror(-ret));
		kms_bo_destroy(&bo);
		return -1;
	}

	/* paint the buffer with colored tiles */
	for (j = 0; j < height; j++) {
		uint32_t *fb_ptr = (uint32_t*)((char*)virtual + j * stride);
		for (i = 0; i < width; i++) {
			div_t d = div(i, width);
			fb_ptr[i] =
				0x00130502 * (d.quot >> 6) +
				0x000a1120 * (d.rem >> 6);
		}
	}

	make_pwetty(virtual, width, height, stride);

	kms_bo_unmap(bo);

	*bo_out = bo;
	*stride_out = stride;
	return 0;
}

static int
create_grey_buffer(struct kms_driver *kms,
		   int width, int height, int *stride_out,
		   struct kms_bo **bo_out)
{
	struct kms_bo *bo;
	int size, ret, stride;
	void *virtual;

	bo = allocate_buffer(kms, width, height, &stride);
	if (!bo)
		return -1;

	ret = kms_bo_map(bo, &virtual);
	if (ret) {
		fprintf(stderr, "failed to map buffer: %s\n",
			strerror(-ret));
		kms_bo_destroy(&bo);
		return -1;
	}

	size = stride * height;
	memset(virtual, 0x77, size);
	kms_bo_unmap(bo);

	*bo_out = bo;
	*stride_out = stride;

	return 0;
}

void
page_flip_handler(int fd, unsigned int frame,
		  unsigned int sec, unsigned int usec, void *data)
{
	struct connector *c;
	unsigned int new_fb_id;
	struct timeval end;
	double t;

	c = data;
	if (c->current_fb_id == c->fb_id[0])
		new_fb_id = c->fb_id[1];
	else
		new_fb_id = c->fb_id[0];

	drmModePageFlip(fd, c->crtc, new_fb_id,
			DRM_MODE_PAGE_FLIP_EVENT, c);
	c->current_fb_id = new_fb_id;
	c->swap_count++;
	if (c->swap_count == 60) {
		gettimeofday(&end, NULL);
		t = end.tv_sec + end.tv_usec * 1e-6 -
			(c->start.tv_sec + c->start.tv_usec * 1e-6);
		fprintf(stderr, "freq: %.02fHz\n", c->swap_count / t);
		c->swap_count = 0;
		c->start = end;
	}
}

static int exynos_gem_map_offset(int fd, struct drm_exynos_gem_map_off *map_off)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_MAP_OFFSET, map_off);
	if (ret < 0) {
		fprintf(stderr, "failed to get buffer offset: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_gem_get_ump(int fd, struct drm_exynos_gem_ump *gem_ump)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_EXPORT_UMP, gem_ump);
	if (ret < 0) {
		fprintf(stderr, "failed to get ump: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_gem_cache_op(int fd,
				struct drm_exynos_gem_cache_op *cache_op)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_CACHE_OP, cache_op);
	if (ret < 0) {
		fprintf(stderr, "failed to cache operation: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_gem_get_phy(int fd,
			struct drm_exynos_gem_get_phy *get_phy)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_GET_PHY, get_phy);
	if (ret < 0) {
		fprintf(stderr, "failed to get physical address: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_gem_phy_imp(int fd,
			struct drm_exynos_gem_phy_imp *phy_imp)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_PHY_IMP, phy_imp);
	if (ret < 0) {
		fprintf(stderr, "failed to import physical memory to gem: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static void
set_mode(struct connector *c, int count, int page_flip)
{
	struct kms_driver *kms;
	struct kms_bo *bo, *other_bo;
	unsigned int fb_id, other_fb_id;
	int i, ret, width, height, x, stride;
	unsigned handle;
	drmEventContext evctx;

	width = 0;
	height = 0;
	for (i = 0; i < count; i++) {
		connector_find_mode(&c[i]);
		if (c[i].mode == NULL)
			continue;
		width += c[i].mode->hdisplay;
		if (height < c[i].mode->vdisplay)
			height = c[i].mode->vdisplay;
	}

	ret = kms_create(fd, &kms);
	if (ret) {
		fprintf(stderr, "failed to create kms driver: %s\n",
			strerror(-ret));
		return;
	}

	if (create_test_buffer(kms, width, height, &stride, &bo))
		return;

	kms_bo_get_prop(bo, KMS_HANDLE, &handle);
	ret = drmModeAddFB(fd, width, height, 24, 32, stride, handle, &fb_id);
	if (ret) {
		fprintf(stderr, "failed to add fb (%ux%u): %s\n",
			width, height, strerror(errno));
		return;
	}

	x = 0;
	for (i = 0; i < count; i++) {
		if (c[i].mode == NULL)
			continue;

		printf("setting mode %s on connector %d, crtc %d\n",
		       c[i].mode_str, c[i].id, c[i].crtc);

		ret = drmModeSetCrtc(fd, c[i].crtc, fb_id, x, 0,
				     &c[i].id, 1, c[i].mode);

		/* XXX: Actually check if this is needed */
		drmModeDirtyFB(fd, fb_id, NULL, 0);

		x += c[i].mode->hdisplay;

		if (ret) {
			fprintf(stderr, "failed to set mode: %s\n", strerror(errno));
			return;
		}
	}

	if (!page_flip)
		return;
	
	if (create_grey_buffer(kms, width, height, &stride, &other_bo))
		return;

	kms_bo_get_prop(other_bo, KMS_HANDLE, &handle);
	ret = drmModeAddFB(fd, width, height, 32, 32, stride, handle,
			   &other_fb_id);
	if (ret) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		return;
	}

	for (i = 0; i < count; i++) {
		if (c[i].mode == NULL)
			continue;

		ret = drmModePageFlip(fd, c[i].crtc, other_fb_id,
				      DRM_MODE_PAGE_FLIP_EVENT, &c[i]);
		if (ret) {
			fprintf(stderr, "failed to page flip: %s\n", strerror(errno));
			return;
		}
		gettimeofday(&c[i].start, NULL);
		c[i].swap_count = 0;
		c[i].fb_id[0] = fb_id;
		c[i].fb_id[1] = other_fb_id;
		c[i].current_fb_id = other_fb_id;
	}

	memset(&evctx, 0, sizeof evctx);
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.vblank_handler = NULL;
	evctx.page_flip_handler = page_flip_handler;
	
	while (1) {
#if 0
		struct pollfd pfd[2];

		pfd[0].fd = 0;
		pfd[0].events = POLLIN;
		pfd[1].fd = fd;
		pfd[1].events = POLLIN;

		if (poll(pfd, 2, -1) < 0) {
			fprintf(stderr, "poll error\n");
			break;
		}

		if (pfd[0].revents)
			break;
#else
		struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(fd, &fds);
		ret = select(fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0) {
			fprintf(stderr, "select timed out or error (ret %d)\n",
				ret);
			continue;
		} else if (FD_ISSET(0, &fds)) {
			break;
		}
#endif

		drmHandleEvent(fd, &evctx);
	}

	kms_bo_destroy(&bo);
	kms_bo_destroy(&other_bo);
	kms_destroy(&kms);
}

extern char *optarg;
extern int optind, opterr, optopt;
static char optstr[] = "ecpmfs:t:v";

void usage(char *name)
{
	fprintf(stderr, "usage: %s [-ecpmfst]\n", name);
	fprintf(stderr, "\t-e\tlist encoders\n");
	fprintf(stderr, "\t-c\tlist connectors\n");
	fprintf(stderr, "\t-p\tlist CRTCs (pipes)\n");
	fprintf(stderr, "\t-m\tlist modes\n");
	fprintf(stderr, "\t-f\tlist framebuffers\n");
	fprintf(stderr, "\t-v\ttest vsynced page flipping\n");
	fprintf(stderr, "\t-s <connector_id>:<mode>\tset a mode\n");
	fprintf(stderr, "\t-s <connector_id>@<crtc_id>:<mode>\tset a mode\n");
	fprintf(stderr, "\t-t <command_type:allocation_type:cache_attr>\tsetup gem allocation\n");
	fprintf(stderr, "\t-t <command_type:cache_type:cache_operation>\tsetup cache operation\n");
	fprintf(stderr, "\t-t <command_type:dmabuf:dmabuf_type>\tsetup drm prime or ump dmabuf\n");
	fprintf(stderr, "\n\tcommand line sentences to -t option\n");
	fprintf(stderr, "\t-t alloc:[contig,noncontig,userptr]:[cache,noncache,writecombine]\n");
	fprintf(stderr, "\t-t cache:[l1,l2,all]:[flush_range,clean_range,inv_range,flush_all,clean_all,inv_all]\n");
	fprintf(stderr, "\t-t [prime,ump]:dmabuf:[export,import,all]\n");
	fprintf(stderr, "\n\tDefault is to dump all info.\n");
	exit(0);
}

#define dump_resource(res) if (res) dump_##res()

static int page_flipping_supported(int fd)
{
	/*FIXME: generic ioctl needed? */
	return 1;
#if 0
	int ret, value;
	struct drm_i915_getparam gp;

	gp.param = I915_PARAM_HAS_PAGEFLIPPING;
	gp.value = &value;

	ret = drmCommandWriteRead(fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
	if (ret) {
		fprintf(stderr, "drm_i915_getparam: %m\n");
		return 0;
	}

	return *gp.value;
#endif
}

static void test_exynos_gem(struct connector *c, int count, int page_flip)
{
	struct drm_exynos_gem_map_off map_off1;

	struct drm_exynos_gem_cache_op cache_op;

	struct ump_uk_dmabuf ump_dmabuf;
	int ump_fd;

	void *vaddr;

	struct kms_driver *kms;
	struct kms_bo *other_bo;
	int i, ret, width, height, x, stride;
	unsigned int fb_id, other_fb_id;
	uint32_t handle;
	drmEventContext evctx;
	void *usr_addr1;
	int prime_fd;

	struct exynos_device *dev;
	struct exynos_bo *bo;
	unsigned int gem_size, gem_flags;

#if 0
#ifdef UMP_TEST
	struct drm_exynos_gem_ump gem_ump;
#endif
#ifdef GET_PHY_TEST
	/* temporary codes. */
	struct drm_exynos_gem_get_phy get_phy;
	struct drm_exynos_gem_phy_imp phy_imp;
#endif
#ifdef VIDI_TEST
	struct drm_exynos_vidi_connection vidi;
#endif
#ifdef GEMGET_TEST
	struct drm_exynos_gem_info info;
#endif
#ifdef VIDI_TEST
	vidi.connection = 1;
	exynos_vidi_connection(fd, &vidi);
#endif
#endif

	dev = exynos_device_create(fd);
	if (!dev)
		return;

	exynos_test_dump();

	width = 0;
	height = 0;
	for (i = 0; i < count; i++) {
		connector_find_mode(&c[i]);
		if (c[i].mode == NULL)
			continue;
		width += c[i].mode->hdisplay;
		if (height < c[i].mode->vdisplay)
			height = c[i].mode->vdisplay;
	}

	if (exynos_test.cmd_type & CMD_ALLOC) {
		if (exynos_test.mem_type & ALLOC_USERPTR) {
			vaddr = malloc(DEFAULT_SIZE);
			if (!vaddr)
				return;

			ret = exynos_userptr(dev, vaddr, DEFAULT_SIZE, &handle);
			if (ret < 0)
				return;

			usr_addr1 = vaddr;
			printf("userptr handle = %d\n", handle);
		} else {
			unsigned int flags;

			/* flag memory type. */
			if (exynos_test.mem_type & ALLOC_CONTIG)
				flags = EXYNOS_BO_CONTIG;
			else
				flags = EXYNOS_BO_NONCONTIG;

			/* flag cache mapping type. */
			if (exynos_test.map_type & ALLOC_MAP_C)
				flags |= EXYNOS_BO_CACHABLE;
			else if (exynos_test.map_type & ALLOC_MAP_NC)
				flags |= EXYNOS_BO_NONCACHABLE;
			else
				flags |= EXYNOS_BO_WC;

			bo = exynos_bo_create(dev, DEFAULT_SIZE, flags);
			if (!bo)
				return;

			handle = bo->handle;

			printf("gem handle = %d\n", bo->handle);
		}
	}

#ifdef DIRECT_MAP
	if (!(exynos_test.mem_type & ALLOC_USERPTR)) {
		printf("handle = %d, size = 0x%x\n", bo->handle, bo->size);

		usr_addr1 = exynos_bo_map(bo);
		if (!usr_addr1)
			return;

		memset(usr_addr1, 0x88, bo->size);
	}
#endif
#ifdef INDIRECT_MAP
	/* FIXME */
	if (!(exynos_test.mem_type & ALLOC_USERPTR)) {
		map_off1.handle = handle;

		/* get map offset. */
		ret = exynos_gem_map_offset(fd, &map_off1);
		if (ret < 0)
			return;

		usr_addr1 = mmap(0, gem1.size, PROT_READ | PROT_WRITE,
				MAP_SHARED, fd, map_off1.offset);

		memset(usr_addr1, 0x88, gem1.size);
	}
#endif

	if (exynos_test.cmd_type & CMD_CACHE) {
		cache_op.flags = 0;

		/* flag cache units to do cache operation. */
		if (exynos_test.cache_type & CACHE_L1)
			cache_op.flags |= EXYNOS_DRM_L1_CACHE;
		if (exynos_test.cache_type & CACHE_L2)
			cache_op.flags |= EXYNOS_DRM_L2_CACHE;
		if (exynos_test.cache_type & CACHE_ALL_CORES)
			cache_op.flags |= EXYNOS_DRM_ALL_CORES;

		/* flag cache operations. */
		if (exynos_test.cache_op_type & CACHE_CLEAN_RANGE)
			cache_op.flags |= EXYNOS_DRM_CACHE_CLN_RANGE;
		if (exynos_test.cache_op_type & CACHE_INV_RANGE)
			cache_op.flags |= EXYNOS_DRM_CACHE_INV_RANGE;
		if (!(exynos_test.cache_op_type & CACHE_CLEAN_RANGE)) {
			cache_op.flags &= ~EXYNOS_DRM_CACHE_CLN_RANGE;
			cache_op.flags |= EXYNOS_DRM_CACHE_CLN_ALL;
		}
		if (!(exynos_test.cache_op_type & CACHE_INV_RANGE)) {
			cache_op.flags &= ~EXYNOS_DRM_CACHE_INV_RANGE;
			cache_op.flags |= EXYNOS_DRM_CACHE_INV_ALL;
		}

		cache_op.usr_addr = usr_addr1 + exynos_test.start_offset;
		cache_op.size = exynos_test.end_offset -
				exynos_test.start_offset;

		exynos_gem_cache_op(fd, &cache_op);
	}

#ifdef GEMGET_TEST
	ret = exynos_bo_gem_info(dev, bo->handle, &gem_size, &gem_flags);
	if (ret < 0)
		return;

	printf("[gem get] flags = 0x%x, size = 0x%x\n", gem_flags, gem_size);
#endif

	if (exynos_test.cmd_type & CMD_PRIME) {
		if (exynos_test.dmabuf_type & DMABUF_EXPORT) {
			ret = exynos_prime_handle_to_fd(dev, bo->handle,
								&prime_fd);
			if (ret < 0)
				return;

			printf("handle to fd : handle(0x%x) ==> fd(%d)\n",
								bo->handle,
								prime_fd);
		}

		if (exynos_test.dmabuf_type & DMABUF_IMPORT) {
			uint32_t gem_handle;

			ret = exynos_prime_fd_to_handle(dev, prime_fd, &gem_handle);
			if (ret < 0)
				return;

			printf("fd to handle : fd(%d) ==> handle(0x%x)\n",
								prime_fd,
								gem_handle);
		}
	}

	if (exynos_test.cmd_type & CMD_UMP) {
		ump_fd = open("/dev/ump", O_RDWR);
		if (ump_fd < 0) {
			printf("failed to open ump.\n");
			return;
		}

		memset(&ump_dmabuf, 0, sizeof(struct ump_uk_dmabuf));

		if (exynos_test.dmabuf_type & DMABUF_IMPORT) {
			ump_dmabuf.fd = prime_fd;
			ret = ioctl(ump_fd, UMP_IOC_DMABUF_IMPORT, &ump_dmabuf);
			if (ret < 0) {
				printf("failed to ioctl ump.\n");
				return;
			}

			printf("ump handle = 0x%x\n", ump_dmabuf.ump_handle);
		}

		close(ump_fd);
	}

#if 0
#ifdef GET_PHY_TEST
	memset(&get_phy, 0, sizeof(struct drm_exynos_gem_get_phy));
	get_phy.gem_handle = handle;
	exynos_gem_get_phy(fd, &get_phy);
	printf("get_phy->phy_addr = 0x%llx, get_phy->size = 0x%llx\n",
			get_phy.phy_addr, get_phy.size);

	memset(&phy_imp, 0, sizeof(struct drm_exynos_gem_phy_imp));
	phy_imp.phy_addr = get_phy.phy_addr;
	phy_imp.size = get_phy.size;
	exynos_gem_phy_imp(fd, &phy_imp);
	printf("phy_imp->handle = 0x%x\n", phy_imp.gem_handle);
#endif

#ifdef UMP_TEST
	/* get secure id for ump. */
	gem_ump.gem_handle = handle;

	exynos_gem_get_ump(fd, &gem_ump);

	printf("secure id = %d\n", gem_ump.secure_id);
#endif
#endif
	stride = width * 4;

	ret = drmModeAddFB(fd, width, height, 32, 32, stride, handle, &fb_id);
	if (ret) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		goto err;
	}

	x = 0;
	for (i = 0; i < count; i++) {
		if (c[i].mode == NULL)
			continue;

		printf("setting mode %s on connector %d, crtc %d\n",
		       c[i].mode_str, c[i].id, c[i].crtc);

		ret = drmModeSetCrtc(fd, c[i].crtc, fb_id, x, 0,
				     &c[i].id, 1, c[i].mode);

		/* XXX: Actually check if this is needed */
		drmModeDirtyFB(fd, fb_id, NULL, 0);

		x += c[i].mode->hdisplay;

		if (ret) {
			fprintf(stderr, "failed to set mode: %s\n", strerror(errno));
			return;
		}
	}

	if (!page_flip)
		return;

	ret = kms_create(fd, &kms);
	if (ret) {
		fprintf(stderr, "failed to create kms driver: %s\n",
			strerror(-ret));
		return;
	}

	if (create_test_buffer(kms, width, height, &stride, &other_bo))
		return;

	kms_bo_get_prop(other_bo, KMS_HANDLE, &handle);
	ret = drmModeAddFB(fd, width, height, 32, 32, stride, handle,
			   &other_fb_id);
	if (ret) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		return;
	}

	for (i = 0; i < count; i++) {
		if (c[i].mode == NULL)
			continue;

		ret = drmModePageFlip(fd, c[i].crtc, other_fb_id,
				      DRM_MODE_PAGE_FLIP_EVENT, &c[i]);
		if (ret) {
			fprintf(stderr, "failed to page flip: %s\n", strerror(errno));
			return;
		}
		gettimeofday(&c[i].start, NULL);
		c[i].swap_count = 0;
		c[i].fb_id[0] = fb_id;
		c[i].fb_id[1] = other_fb_id;
		c[i].current_fb_id = other_fb_id;
	}

	memset(&evctx, 0, sizeof evctx);
	evctx.version = DRM_EVENT_CONTEXT_VERSION;
	evctx.vblank_handler = NULL;
	evctx.page_flip_handler = page_flip_handler;

	while (1) {
		struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(fd, &fds);
		ret = select(fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0) {
			fprintf(stderr, "select timed out or error (ret %d)\n",
				ret);
			continue;
		} else if (FD_ISSET(0, &fds)) {
			break;
		}

		drmHandleEvent(fd, &evctx);
	}

	kms_bo_destroy(&other_bo);
	kms_destroy(&kms);

err:
	if (exynos_test.mem_type == ALLOC_USERPTR) {
		struct drm_gem_close gem_close;

		gem_close.handle = handle;

		free(usr_addr1);
		drmIoctl(fd, DRM_IOCTL_GEM_CLOSE, &gem_close);
	} else
		exynos_bo_destroy(bo);
}


static int setup_exynos_gem_map_type(void)
{
	if (strncmp(exynos_test.scmd[2], "cache", 5) == 0)
		exynos_test.map_type = ALLOC_MAP_C;
	else if (strncmp(exynos_test.scmd[2], "noncache", 8) == 0)
		exynos_test.map_type = ALLOC_MAP_NC;
	else if (strncmp(exynos_test.scmd[2], "writecombine", 12) == 0)
		exynos_test.map_type = ALLOC_MAP_WC;
	else {
		printf("invalid map type.\n");
		return -EINVAL;
	}

	return 0;
}

static int setup_exynos_gem_cache_op_type(void)
{
	if (strncmp(exynos_test.scmd[2], "clean_range", 11) == 0)
		exynos_test.cache_op_type |= CACHE_CLEAN_RANGE;
	else if (strncmp(exynos_test.scmd[2], "inv_range", 9) == 0)
		exynos_test.cache_op_type |= CACHE_INV_RANGE;
	else if (strncmp(exynos_test.scmd[2], "flush_range", 11) == 0)
		exynos_test.cache_op_type |= CACHE_FLUSH_RANGE;
	else if (strncmp(exynos_test.scmd[2], "clean_all", 9) == 0)
		exynos_test.cache_op_type &= ~CACHE_CLEAN_RANGE; /* all */
	else if (strncmp(exynos_test.scmd[2], "inv_all", 7) == 0)
		exynos_test.cache_op_type &= ~CACHE_INV_RANGE; /* all */
	else if (strncmp(exynos_test.scmd[2], "flush_all", 9) == 0)
		exynos_test.cache_op_type &= ~CACHE_FLUSH_RANGE; /* all */
	else {
		printf("invalid cache op type.\n");
		return -EINVAL;
	}

	return 0;
}

static int setup_exynos_gem_dmabuf_type(void)
{
	if (strncmp(exynos_test.scmd[2], "export", 6) == 0)
		exynos_test.dmabuf_type |= DMABUF_EXPORT;
	else if (strncmp(exynos_test.scmd[2], "import", 6) == 0)
		exynos_test.dmabuf_type |= DMABUF_IMPORT;
	else if (strncmp(exynos_test.scmd[2], "all", 3) == 0)
		exynos_test.dmabuf_type |= DMABUF_ALL;
	else {
		printf("invalid dmabuf type.\n");
		return -EINVAL;
	}

	return 0;
}

static int setup_exynos_gem_test(void)
{
	if (strncmp(exynos_test.scmd[0], "alloc", 5) == 0) {
		exynos_test.cmd_type |= CMD_ALLOC;

		if (strncmp(exynos_test.scmd[1], "contig", 5) == 0)
			exynos_test.mem_type = ALLOC_CONTIG;
		else if (strncmp(exynos_test.scmd[1], "noncontig", 8) == 0)
			exynos_test.mem_type = ALLOC_NONCONTIG;
		else if (strncmp(exynos_test.scmd[1], "userptr", 7) == 0)
			exynos_test.mem_type = ALLOC_USERPTR;
		else {
			printf("invalid option type.\n");
			return -EINVAL;
		}

		return setup_exynos_gem_map_type();
	} else if (strncmp(exynos_test.scmd[0], "cache", 5) == 0) {
		exynos_test.cmd_type |= CMD_CACHE;

		if (strncmp(exynos_test.scmd[1], "l1", 2) == 0) {
			exynos_test.cache_type |= CACHE_L1;
		} else if (strncmp(exynos_test.scmd[1], "l2", 2) == 0) {
			exynos_test.cache_type |= CACHE_L2;
		} else if (strncmp(exynos_test.scmd[1], "all", 3) == 0) {
			exynos_test.cache_type |= CACHE_ALL_CACHES;
		} else if (strncmp(exynos_test.scmd[1], "all_cores", 9) == 0) {
			exynos_test.cache_type |= CACHE_ALL_CACHES_CORES;
		} else {
			printf("invalid option type.\n");
			return -EINVAL;
		}

		return setup_exynos_gem_cache_op_type();
	} else if ((strncmp(exynos_test.scmd[0], "prime", 5) == 0) ||
			(strncmp(exynos_test.scmd[0], "ump", 3) == 0)) {
		exynos_test.cmd_type |= (strncmp(exynos_test.scmd[0],
						"prime", 5) == 0) ?
						CMD_PRIME : CMD_UMP;

		if (strncmp(exynos_test.scmd[1], "dmabuf", 6) == 0)
			return setup_exynos_gem_dmabuf_type();
		else {
			printf("invalid option type.\n");
			return -EINVAL;
		}
	} else {
		printf("invalid command type.\n");
		return -EINVAL;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int c;
	int encoders = 0, connectors = 0, crtcs = 0, framebuffers = 0;
	int test_vsync = 0;
	char *modules[] = { "exynos", "i915", "radeon", "nouveau" };
	char *modeset = NULL;
	int i, count = 0, private_test = 0, cmd_cnt = 0, ret = 0;
	struct connector con_args[2];

	opterr = 0;

	while ((c = getopt(argc, argv, optstr)) != -1) {
		switch (c) {
		case 'e':
			encoders = 1;
			break;
		case 'c':
			connectors = 1;
			break;
		case 'p':
			crtcs = 1;
			break;
		case 'm':
			modes = 1;
			break;
		case 'f':
			framebuffers = 1;
			break;
		case 'v':
			test_vsync = 1;
			break;
		case 's':
			modeset = strdup(optarg);
			con_args[count].crtc = -1;
			if (sscanf(optarg, "%d:%64s",
				   &con_args[count].id,
				   con_args[count].mode_str) != 2 &&
			    sscanf(optarg, "%d@%d:%64s",
				   &con_args[count].id,
				   &con_args[count].crtc,
				   con_args[count].mode_str) != 3)
				usage(argv[0]);
			count++;
			break;
		case 't':
			modeset = strdup(optarg);
			exynos_test.scmd[cmd_cnt++] = strtok(modeset, ":~");
			while (exynos_test.scmd[cmd_cnt++] = strtok(NULL, ":~"));

			cmd_cnt--;
			if (cmd_cnt != 3 && cmd_cnt != 5)
				usage(argv[0]);

			ret = setup_exynos_gem_test();
			if (ret < 0)
				return ret;

			private_test = 1;
			cmd_cnt = 0;
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if (argc == 1)
		encoders = connectors = crtcs = modes = framebuffers = 1;

	for (i = 0; i < ARRAY_SIZE(modules); i++) {
		printf("trying to load module %s...", modules[i]);
		fd = drmOpen(modules[i], NULL);
		if (fd < 0) {
			printf("failed.\n");
		} else {
			printf("success.\n");
			break;
		}
	}

	if (test_vsync && !page_flipping_supported(fd)) {
		fprintf(stderr, "page flipping not supported by drm.\n");
		return -1;
	}

	if (i == ARRAY_SIZE(modules)) {
		fprintf(stderr, "failed to load any modules, aborting.\n");
		return -1;
	}

	resources = drmModeGetResources(fd);
	if (!resources) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		drmClose(fd);
		return 1;
	}

	dump_resource(encoders);
	dump_resource(connectors);
	dump_resource(crtcs);
	dump_resource(framebuffers);

	if (count > 0) {
		if (private_test)
			test_exynos_gem(con_args, count, test_vsync);
		else
			set_mode(con_args, count, test_vsync);

		getchar();
	}

	drmModeFreeResources(resources);

	return 0;
}
