#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include "xf86drm.h"
#include "xf86drmMode.h"
#include "libkms.h"
#include "exynos_drm.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

drmModeRes *resources;
int fd;

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

struct fb_data {
	struct kms_driver *kms;
	struct kms_bo *bo;
	unsigned int fb_id;
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
		int width, int height, unsigned *stride)
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

static int
create_grey_buffer(struct kms_driver *kms,
		   int width, int height, int *stride_out,
		   struct kms_bo **bo_out, int color)
{
	struct kms_bo *bo;
	int size, ret;
	unsigned stride;
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
	memset(virtual, color, size);
	kms_bo_unmap(bo);

	*bo_out = bo;
	*stride_out = stride;

	return 0;
}

static int
connector_find_plane(struct connector *c)
{
	drmModePlaneRes *plane_resources;
	drmModePlane *ovr;
	uint32_t id = 0;
	int i;

	plane_resources = drmModeGetPlaneResources(fd);
	if (!plane_resources) {
		fprintf(stderr, "drmModeGetPlaneResources failed: %s\n",
			strerror(errno));
		return 0;
	}

	for (i = 0; i < plane_resources->count_planes; i++) {
		ovr = drmModeGetPlane(fd, plane_resources->planes[i]);
		if (!ovr) {
			fprintf(stderr, "drmModeGetPlane failed: %s\n",
				strerror(errno));
			continue;
		}

		if (ovr->possible_crtcs & (1<<i)) {
			id = ovr->plane_id;
			drmModeFreePlane(ovr);
			break;
		}
		drmModeFreePlane(ovr);
	}

	return id;
}

static int
connector_find_plane2(struct connector *c, unsigned int *plane_id)
{
	drmModePlaneRes *plane_resources;
	drmModePlane *ovr;
	int i;

	plane_resources = drmModeGetPlaneResources(fd);
	if (!plane_resources) {
		fprintf(stderr, "drmModeGetPlaneResources failed: %s\n",
			strerror(errno));
		return -1;
	}

	for (i = 0; i < plane_resources->count_planes; i++) {
		plane_id[i] = 0;

		ovr = drmModeGetPlane(fd, plane_resources->planes[i]);
		if (!ovr) {
			fprintf(stderr, "drmModeGetPlane failed: %s\n",
				strerror(errno));
			continue;
		}

		if (ovr->possible_crtcs & (1 << 0))
			plane_id[i] = ovr->plane_id;
		drmModeFreePlane(ovr);
	}

	return 0;
}

static struct fb_data *make_fb(int w, int h, int color)
{
	struct fb_data *fb_data;
	struct kms_driver *kms;
	struct kms_bo *bo;
	unsigned int fb_id;
	unsigned handle;
	int stride;
	int err;

	fb_data = malloc(sizeof(struct fb_data));
	if (!fb_data)
		return NULL;

	memset(fb_data, 0, sizeof(struct fb_data));

	err = kms_create(fd, &kms);
	if (err) {
		fprintf(stderr, "failed to create kms driver: %s\n",
			strerror(-err));
		goto err_alloc;
	}

	if (create_grey_buffer(kms, w, h, &stride, &bo, color))
		goto err_alloc;

	kms_bo_get_prop(bo, KMS_HANDLE, &handle);
	err = drmModeAddFB(fd, w, h, 32, 32, stride, handle, &fb_id);
	if (err) {
		fprintf(stderr, "failed to add fb: %s\n", strerror(errno));
		goto err_alloc;
	}

	fb_data->fb_id = fb_id;
	fb_data->kms = kms;
	fb_data->bo = bo;

	return fb_data;

err_alloc:
	free(fb_data);
	return NULL;
}

static int exynos_plane_set_zpos(int fd, unsigned int plane_id, int zpos)
{
	struct drm_exynos_plane_set_zpos zpos_req;
	int ret;

	zpos_req.plane_id = plane_id;
	zpos_req.zpos = zpos;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_PLANE_SET_ZPOS, &zpos_req);
	if (ret < 0) {
		fprintf(stderr, "failed to set plane zpos: %s\n",
				strerror(-ret));
		return ret;
	}

	return 0;
}

#define PLANE_NR	5
static void planetest_start(int nr, int start_plane, int crtc, int connector)
{
	struct connector c;
	struct fb_data *fb_data[PLANE_NR];
	unsigned int plane_id[PLANE_NR];
	int height;
	int width;
	int x;
	int y;
	int i;
	int ret;

	c.id = connector;
	c.crtc = crtc;

	if (nr < 1 || nr > PLANE_NR) {
		fprintf(stderr, "wrong plane count\n");
		return;
	}

	if (start_plane < 0 || start_plane > (nr -1) ) {
		fprintf(stderr, "Wrong start plane\n");
		return;
	}

	connector_find_mode(&c);

	if (c.mode == NULL) {
		fprintf(stderr, "mode is NULL\n");
		return;
	}

	width = c.mode->hdisplay;
	height = c.mode->vdisplay;

	width /= 8;
	height /= 8;
	x = width;
	y = height;

	ret = connector_find_plane2(&c, plane_id);
	if (ret < 0)
		goto err;

	for (i = start_plane; i < nr; i++) {
		if (!plane_id[i])
			continue;

		fb_data[i] = make_fb(width, height, 0x30 + 0x20 * i);
		if (!fb_data[i])
			return;

		if (exynos_plane_set_zpos(fd, plane_id[i], i))
			goto err;

		if (drmModeSetPlane(fd, plane_id[i], c.crtc, fb_data[i]->fb_id,
					x, y, width, height,
					0, 0, width, height)) {
			fprintf(stderr, "failed to enable plane: %s\n",
					strerror(errno));
			goto err;
		}

		x += width - 30;
		y += height - 30;
		width += 40;
		height += 40;
	}

	getchar();

err:
	for (i = start_plane; i < nr; i++) {
		if (fb_data[i]) {
			kms_bo_destroy(&fb_data[i]->bo);
			kms_destroy(&fb_data[i]->kms);
			free(fb_data[i]);
		}

		if (!plane_id[i])
			continue;

		if (drmModeSetPlane(fd, plane_id[i], c.crtc,
				0, 0, 0, /* bufferId, crtc_x, crtc_y */
				0, 0, /* crtc_w, crtc_h */
				0, 0, 0, 0 /* src_XXX */)) {
			fprintf(stderr, "failed to enable plane: %s\n",
					strerror(errno));
			goto err;
		}
	}
}

static void help(void)
{
	printf("Usage: ./planetest [OPTION]\n"
	       " [OPTION]\n"
	       " -s <connector_id>@<crtc_id>/<plane count>/<start_plane>\n"
	       " -h Usage\n"
	       "\n"
	       "Default: connector 11, crtc 3, plane_count 5, start_plane 1\n");
	exit(0);
}

static char optstr[] = "hs:";

int main(int argc, char **argv)
{
	char *modules[] = { "exynos-drm" };
	int opt;
	int i, connector_id, nr, start_plane;
	uint32_t crtc_id;

	// default option (LCD)
	crtc_id = 3;
	connector_id = 11;
	nr = 5;
	start_plane = 1;

	/* parse args */
	opterr = 0;
	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
		case 's':
			if (sscanf(optarg, "%d@%d/%d/%d",
				   &connector_id,
				   &crtc_id,
				   &nr,
				   &start_plane) != 4) {
				help();
				break;
			}
			break;
		case 'h':
		default:
			help();
			break;
		}
		break;
	}

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

	resources = drmModeGetResources(fd);
	if (!resources) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		drmClose(fd);
		return 1;
	}

	planetest_start(nr, start_plane, crtc_id, connector_id);

	/* TODO */

	drmModeFreeResources(resources);

	return 0;
}
