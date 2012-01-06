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
#include <errno.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>

#include "xf86drm.h"
#include "xf86drmMode.h"
#include "exynos_drm.h"

#ifdef HAVE_CAIRO
#include <math.h>
#include <cairo.h>
#endif

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) + 1))

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

#define CACHE_TEST

#define USERPTR_TEST

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

static int exynos_gem_create(int fd, struct drm_exynos_gem_create *gem)
{
	int ret;

	if (!gem) {
		fprintf(stderr, "gem object is null.\n");
		return -EFAULT;
	}

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_CREATE, gem);
	if (ret < 0) {
		fprintf(stderr, "failed to create gem buffer: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
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

static int exynos_gem_mmap(int fd, struct drm_exynos_gem_mmap *in_mmap)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_MMAP, in_mmap);
	if (ret < 0) {
		fprintf(stderr, "failed to get buffer offset: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_gem_close(int fd, struct drm_gem_close *gem_close)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_GEM_CLOSE, gem_close);
	if (ret < 0) {
		fprintf(stderr, "failed to close gem buffer: %s\n",
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

static int exynos_gem_userptr_imp(int fd,
			struct drm_exynos_gem_userptr_imp *imp)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_USERPTR_IMP, imp);
	if (ret < 0) {
		fprintf(stderr, "failed to cache operation: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

static void
set_mode(struct connector *c, int count, int page_flip)
{
	struct drm_exynos_gem_create gem1, gem2;
	struct drm_exynos_gem_map_off map_off1, map_off2;
	struct drm_exynos_gem_mmap mmap1;
	struct drm_exynos_gem_cache_op cache_op;
	struct drm_exynos_gem_ump gem_ump;
	struct drm_exynos_gem_userptr_imp imp;
	struct drm_gem_close args;
	unsigned int fb_id, other_fb_id;
	int i, ret, width, height, x, stride;
	unsigned handle;
	void *usr_addr1, *usr_addr2;
	drmEventContext evctx;
	unsigned int tmp;

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

#ifdef CACHE_TEST
	cache_op.flags = 0;
	cache_op.flags = (EXYNOS_DRM_ALL_CACHE | EXYNOS_DRM_CACHE_FSH);

	exynos_gem_cache_op(fd, &cache_op);
#endif

	/* allocate gem buffer. */
	gem1.size = 1024 * 600 * 4;

	ret = exynos_gem_create(fd, &gem1);
	if (ret < 0)
		return;

	handle = gem1.handle;

#ifdef DIRECT_MAP
	mmap1.handle = handle;
	mmap1.size = gem1.size;
	ret = exynos_gem_mmap(fd, &mmap1);
	if (ret < 0)
		return;

	usr_addr1 = mmap1.mapped;
	memset(usr_addr1, 0x88, mmap1.size);
#endif
#ifdef INDIRECT_MAP
	map_off1.handle = handle;

	/* get map offset. */
	ret = exynos_gem_map_offset(fd, &map_off1);
	if (ret < 0)
		return;

	usr_addr1 = mmap(0, gem1.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map_off1.offset);

	memset(usr_addr1, 0x88, gem1.size);
#endif

#ifdef USERPTR_TEST
	memset(&imp, 0, sizeof(struct drm_exynos_gem_userptr_imp));

	imp.size = mmap1.size;
	imp.user_ptr = usr_addr1;

	printf("size = 0x%x, addr = 0x%x\n", imp.size, imp.user_ptr);

	ret = exynos_gem_userptr_imp(fd, &imp);
	if (ret < 0)
		return;

	printf("handle = 0x%x\n", imp.handle);
#endif

	/* get secure id for ump. */
	gem_ump.gem_handle = handle;

	exynos_gem_get_ump(fd, &gem_ump);

	printf("secure id = %d\n", gem_ump.secure_id);

	ret = drmModeAddFB(fd, width, height, 32, 32, stride, handle, &fb_id);
	if (ret) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
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
		x += c[i].mode->hdisplay;

		if (ret) {
			fprintf(stderr, "failed to set mode: %s\n", strerror(errno));
			return;
		}
	}

	if (!page_flip)
		return;

	/* for second gem object, INDIRECT_MODE would be used. */

	gem2.size = 1024 * 600 * 4;

	/* allocate gem buffer. */
	ret = exynos_gem_create(fd, &gem2);
	if (ret < 0)
		return;

	handle = map_off2.handle = gem2.handle;

	/* get map offset. */
	ret = exynos_gem_map_offset(fd, &map_off2);
	if (ret < 0)
		return;

	usr_addr2 = mmap(0, gem2.size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, map_off2.offset);

	memset(usr_addr2, 0x0, gem2.size);
	ret = drmModeAddFB(fd, width, height, 32, 32, stride, handle,
			   &other_fb_id);
	if (ret) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		return;
	}

	for (i = 0; i < count; i++) {
		if (c[i].mode == NULL)
			continue;

		drmModePageFlip(fd, c[i].crtc, other_fb_id,
				DRM_MODE_PAGE_FLIP_EVENT, &c[i]);
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

	/* release user space memroy. */
#if defined(INDIRECT_MAP) || defined(DIRECT_MAP)
	munmap(usr_addr1, mmap1.size);
#endif
	munmap(usr_addr2, gem2.size);

	args.handle = gem1.handle;

	/* relese gem buffer. */
	ret = exynos_gem_close(fd, &args);
	if (ret < 0)
		return;

	args.handle = gem2.handle;
	ret = exynos_gem_close(fd, &args);
	if (ret < 0)
		return;

#ifdef USERPTR_TEST
	args.handle = imp.handle;
	ret = exynos_gem_close(fd, &args);
	if (ret < 0)
		return;
#endif
}

extern char *optarg;
extern int optind, opterr, optopt;
static char optstr[] = "ecpmfs:v";

void usage(char *name)
{
	fprintf(stderr, "usage: %s [-ecpmf]\n", name);
	fprintf(stderr, "\t-e\tlist encoders\n");
	fprintf(stderr, "\t-c\tlist connectors\n");
	fprintf(stderr, "\t-p\tlist CRTCs (pipes)\n");
	fprintf(stderr, "\t-m\tlist modes\n");
	fprintf(stderr, "\t-f\tlist framebuffers\n");
	fprintf(stderr, "\t-v\ttest vsynced page flipping\n");
	fprintf(stderr, "\t-s <connector_id>:<mode>\tset a mode\n");
	fprintf(stderr, "\t-s <connector_id>@<crtc_id>:<mode>\tset a mode\n");
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

int main(int argc, char **argv)
{
	int c;
	int encoders = 0, connectors = 0, crtcs = 0, framebuffers = 0;
	int test_vsync = 0;
	char *modules[] = { "exynos-drm", "i915", "radeon", "nouveau" };
	char *modeset = NULL;
	int i, count = 0;
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
		set_mode(con_args, count, test_vsync);
		getchar();
	}

	drmModeFreeResources(resources);

	return 0;
}