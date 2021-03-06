#ifndef __ROTTEST_H__
#define __ROTTEST_H__

#include "xf86drm.h"
#include "xf86drmMode.h"

#define MAX_LOOP 20

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

extern int fd;

extern void connector_find_mode(struct connector *c);

#endif
