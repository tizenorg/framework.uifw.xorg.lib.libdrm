#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xf86drm.h"
#include "xf86drmMode.h"
#include "exynos_drm.h"
#include "drm_fourcc.h"
#include "g2d.h"

#define DRM_MODULE_NAME		"exynos"

struct connector {
	uint32_t id;
	char mode_str[64];
	drmModeModeInfo *mode;
	drmModeEncoder *encoder;
	int crtc;
	int plane_zpos;
	unsigned int fb_id[2], current_fb_id;
	struct timeval start;

	int swap_count;
};

struct drm_buffer {
	struct drm_exynos_gem_create	gem;
	struct drm_exynos_gem_mmap	gem_mmap;
};

struct drm_fb {
	uint32_t			id;
	struct drm_buffer		drm_buffer;
};

struct drm_desc {
	int				fd;
	struct connector		connector;
	int				plane_id[5];
	int				width;
	int				height;
};

enum format {
	FMT_RGB565,
	FMT_RGB888,
	FMT_NV12M,
};

static void connector_find_mode(int fd, struct connector *c,
				drmModeRes *resources)
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

static int connector_find_plane(int fd, unsigned int *plane_id)
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

static int exynos_g2d_get_ver(int fd, struct drm_exynos_g2d_get_ver *ver)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_G2D_GET_VER, ver);
	if (ret < 0) {
		fprintf(stderr, "failed to get version: %s\n", strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_g2d_set_cmdlist(int fd,
				  struct drm_exynos_g2d_set_cmdlist *cmdlist)
{
	int ret;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_G2D_SET_CMDLIST, cmdlist);
	if (ret < 0) {
		fprintf(stderr, "failed to set cmdlist: %s\n", strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_g2d_exec(int fd, int async)
{
	struct drm_exynos_g2d_exec exec;
	int ret;

	exec.async = async;
	ret = ioctl(fd, DRM_IOCTL_EXYNOS_G2D_EXEC, &exec);
	if (ret < 0) {
		fprintf(stderr, "failed to execute: %s\n", strerror(-ret));
		return ret;
	}

	return 0;
}

static int exynos_gem_create(int fd, struct drm_exynos_gem_create *gem)
{
	int ret;

	if (!gem)
		return -EINVAL;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_CREATE, gem);
	if (ret < 0)
		perror("ioctl failed\n");

	return ret;
}

static int exynos_gem_map_offset(int fd, struct drm_exynos_gem_map_off *map_off)
{
	int ret;

	if (!map_off)
		return -EINVAL;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_MAP_OFFSET, map_off);
	if (ret < 0)
		perror("ioctl failed\n");

	return ret;
}

static int exynos_gem_mmap(int fd, struct drm_exynos_gem_mmap *in_mmap)
{
	int ret;

	if (!in_mmap)
		return -EINVAL;

	ret = ioctl(fd, DRM_IOCTL_EXYNOS_GEM_MMAP, in_mmap);
	if (ret < 0)
		perror("ioctl failed\n");

	return ret;
}

static int exynos_gem_close(int fd, struct drm_gem_close *gem_close)
{
	int ret;

	if (!gem_close)
		return -EINVAL;

	ret = ioctl(fd, DRM_IOCTL_GEM_CLOSE, gem_close);
	if (ret < 0)
		perror("ioctl failed\n");

	return ret;
}

static struct drm_desc *drm_alloc_desc(void)
{
	struct drm_desc *drm_desc;

	drm_desc = malloc(sizeof(struct drm_desc));
	if (!drm_desc) {
		perror("memory alloc error\n");
		return NULL;
	}
	memset(drm_desc, 0, sizeof(struct drm_desc));

	return drm_desc;
}

static int drm_open(struct drm_desc *drm_desc)
{
	if (!drm_desc) {
		fprintf(stderr, "drm_desc is NULL\n");
		return -EINVAL;
	}

	drm_desc->fd = drmOpen(DRM_MODULE_NAME, NULL);
	if (drm_desc->fd < 0) {
		printf("Failed to open %s module\n", DRM_MODULE_NAME);
		return drm_desc->fd;
	}

	return 0;
}

static int drm_create_buffer(struct drm_desc *drm_desc, struct drm_fb *drm_fb,
			     int width, int height)
{
	struct drm_buffer *drm_buffer;
	unsigned int num_planes;
	unsigned long size;
	int i;
	int ret;

	if (!drm_desc)
		return -EINVAL;

	size = width * height * 4;

	drm_buffer = &drm_fb->drm_buffer;

	{
		struct drm_exynos_gem_create *gem = &drm_buffer->gem;
		struct drm_exynos_gem_mmap *gem_mmap = &drm_buffer->gem_mmap;

		memset(gem, 0, sizeof(struct drm_exynos_gem_create));
		gem->size = size;

		ret = exynos_gem_create(drm_desc->fd, gem);
		if (ret < 0) {
			printf("failed to create gem\n");
			goto err_gem;
		}

		gem_mmap->handle = gem->handle;
		gem_mmap->size = gem->size;
		ret = exynos_gem_mmap(drm_desc->fd, gem_mmap);
		if (ret < 0) {
			printf("failed to mmap gem directly\n");
			goto err_gem;
		}

		/* init gem buffer */
		memset((void *)(unsigned long)gem_mmap->mapped, 0,
				gem_mmap->size);
	}

	return 0;

err_gem:
	{
		struct drm_exynos_gem_create *gem = &drm_buffer->gem;
		struct drm_gem_close gem_close;

		gem_close.handle = gem->handle;
		exynos_gem_close(drm_desc->fd, &gem_close);
	}

	return ret;
}

static int drm_create_fb(struct drm_desc *drm_desc, struct drm_fb *drm_fb,
			 int width, int height)
{
	unsigned int pixel_format;
	unsigned int num_planes;
	unsigned int pitch;
	int i;
	int j;
	int ret;

	if (!drm_desc)
		return -EINVAL;

	drm_desc->width = width;
	drm_desc->height = height;

	pixel_format = DRM_FORMAT_RGBA8888;
	num_planes = 1;
	pitch = width * 4;

	{
		uint32_t bo[4] = {0,};
		uint32_t pitches[4] = {0,};
		uint32_t offset[4] = {0,};

		ret = drm_create_buffer(drm_desc, drm_fb, width, height);
		if (ret < 0)
			goto err;

		for (j = 0; j < num_planes; j++) {
			struct drm_buffer *drm_buffer = &drm_fb->drm_buffer;
			struct drm_exynos_gem_create *gem = &drm_buffer->gem;

			bo[j] = gem->handle;
			pitches[j] = pitch;
		}

		ret = drmModeAddFB2(drm_desc->fd, width, height, pixel_format,
				bo, pitches, offset, &drm_fb->id,
				0);
		if (ret < 0) {
			perror("failed to add fb\n");
			goto err;
		}
	}

	return 0;

err:
	/* TODO: free buffer */
	return ret;
}

static int drm_set_crtc(struct drm_desc *drm_desc, struct connector *c,
			struct drm_fb *drm_fb)
{
	drmModeRes *resources;
	int ret;

	memcpy(&drm_desc->connector, c, sizeof(struct connector));

	resources = drmModeGetResources(drm_desc->fd);
	if (!resources) {
		fprintf(stderr, "drmModeGetResources failed: %s\n",
			strerror(errno));
		ret = -EFAULT;
		goto err;
	}

	connector_find_mode(drm_desc->fd, &drm_desc->connector, resources);
	drmModeFreeResources(resources);

	ret = drmModeSetCrtc(drm_desc->fd, drm_desc->connector.crtc,
			drm_fb->id, 0, 0,
			&drm_desc->connector.id, 1,
			drm_desc->connector.mode);
	if (ret) {
		fprintf(stderr, "failed to set mode: %s\n", strerror(errno));
		goto err;
	}

	return 0;

err:
	/* TODO */
	return ret;
}

static inline void set_cmd(struct drm_exynos_g2d_cmd *cmd,
				      __u32 offset, __u32 data)
{
	cmd->offset = offset;
	cmd->data = data;
}

static int exynos_g2d_test_solid_fill(struct drm_desc *drm_desc, int x, int y,
				      int color, int gem_handle)
{
	struct drm_exynos_g2d_set_cmdlist cmdlist;
	struct drm_exynos_g2d_cmd cmd[20];
	struct drm_exynos_g2d_cmd cmd_gem[5];
	int nr = 0;
	int gem_nr = 0;
	int ret;

	memset(&cmdlist, 0, sizeof(struct drm_exynos_g2d_set_cmdlist));
	memset(cmd, 0, sizeof(struct drm_exynos_g2d_cmd) * 20);
	memset(cmd_gem, 0, sizeof(struct drm_exynos_g2d_cmd) * 5);

	cmdlist.cmd = cmd;
	cmdlist.cmd_gem = cmd_gem;

	set_cmd(&cmd[nr++], BITBLT_COMMAND_REG, G2D_FAST_SOLID_COLOR_FILL);
	/* [14:10] R, [9:5] G, [4:0] B */
	set_cmd(&cmd[nr++], SF_COLOR_REG, color);

	/* DST */
	set_cmd(&cmd[nr++], DST_SELECT_REG, G2D_SELECT_MODE_FGCOLOR);
	set_cmd(&cmd_gem[gem_nr++], DST_BASE_ADDR_REG, gem_handle);
	set_cmd(&cmd[nr++], DST_STRIDE_REG, 720 * 4);
	set_cmd(&cmd[nr++], DST_COLOR_MODE_REG, G2D_COLOR_FMT_ARGB8888 |
						G2D_ORDER_AXRGB);
	set_cmd(&cmd[nr++], DST_LEFT_TOP_REG, (0 << 16) | 0);
	set_cmd(&cmd[nr++], DST_RIGHT_BOTTOM_REG, (y << 16) | x);
	set_cmd(&cmd[nr++], DST_A8_RGB_EXT_REG, 0);

	cmdlist.cmd_nr = nr;
	cmdlist.cmd_gem_nr = gem_nr;

	cmdlist.event_type = G2D_EVENT_NONSTOP;
	cmdlist.user_data = 1234;

	ret = exynos_g2d_set_cmdlist(drm_desc->fd, &cmdlist);
	if (ret < 0)
		return ret;
}

static int exynos_g2d_event(int fd)
{
	char buffer[1024];
	int len, i;
	struct drm_event *e;
	struct drm_exynos_g2d_event *g2d_event;

	len = read(fd, buffer, sizeof buffer);
	if (len == 0)
		return 0;
	if (len < sizeof *e)
		return -1;

	i = 0;
	while (i < len) {
		e = (struct drm_event *) &buffer[i];
		switch (e->type) {
		case DRM_EXYNOS_G2D_EVENT:
			g2d_event = (struct drm_exynos_g2d_event *) e;
			printf("cmdlist_no: %d\n", g2d_event->cmdlist_no);
			printf("user_data: %lld\n", g2d_event->user_data);
			break;
		default:
			break;
		}
		i += e->length;
	}

	return 0;
}

int main(int argc, char **argv)
{
	struct connector con_args;
	struct drm_desc *drm_desc;
	struct drm_fb drm_fb;
	struct drm_exynos_g2d_get_ver ver;
	int x, y;
	int ret;

	/* default set of connector */
	memset(&con_args, 0, sizeof(struct connector));
	con_args.id = 12;
	con_args.crtc = 3;
	con_args.plane_zpos = -1;
	strcpy(con_args.mode_str, "720x1280");
	x = 720;
	y = 1280;

	drm_desc = drm_alloc_desc();
	if (!drm_desc) {
		ret = -1;
		goto err_free;
	}

	ret = drm_open(drm_desc);
	if (ret < 0)
		goto err_free;

	/* check version */
	ret = exynos_g2d_get_ver(drm_desc->fd, &ver);
	if (ret < 0)
		return ret;

	if (ver.major != 4 || ver.minor != 1) {
		fprintf(stderr, "version(%d.%d) mismatch\n", ver.major, ver.minor);
		return -1;
	}
	printf("g2d hw version: %d.%d\n", ver.major, ver.minor);

	ret = drm_create_fb(drm_desc, &drm_fb, x, y);
	if (ret < 0)
		goto err_drm_close;

	ret = drm_set_crtc(drm_desc, &con_args, &drm_fb);
	if (ret < 0)
		goto err;

	getchar();

	ret = exynos_g2d_test_solid_fill(drm_desc, x, y, 0x1f,
					 drm_fb.drm_buffer.gem.handle);
	if (ret < 0)
		goto err;

	ret = exynos_g2d_exec(drm_desc->fd, 1);
	if (ret < 0)
		return ret;

	while (1) {
		struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
		fd_set fds;
		int ret;

		FD_ZERO(&fds);
		FD_SET(0, &fds);
		FD_SET(drm_desc->fd, &fds);
		ret = select(drm_desc->fd + 1, &fds, NULL, NULL, &timeout);

		if (ret <= 0) {
			fprintf(stderr, "select timed out or error (ret %d)\n",
				ret);
			continue;
		} else if (FD_ISSET(0, &fds)) {
			break;
		}

		exynos_g2d_event(drm_desc->fd);
	}

	getchar();

	/* TODO */

err:
	/* TODO */
err_drm_close:
	drmClose(drm_desc->fd);
err_free:
	if (drm_desc)
		free(drm_desc);

	return ret;
}
