/* exynos_drm.h
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *	Joonyoung Shim <jy0922.shim@samsung.com>
 *	Seung-Woo Kim <sw0312.kim@samsung.com>
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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef _EXYNOS_DRM_H_
#define _EXYNOS_DRM_H_

/**
 * User-desired buffer creation information structure.
 *
 * @size: user-desired memory allocation size.
 *	- this size value would be page-aligned internally.
 * @flags: user request for setting memory type or cache attributes.
 * @handle: returned a handle to created gem object.
 *	- this handle will be set by gem module of kernel side.
 */
struct drm_exynos_gem_create {
	uint64_t size;
	unsigned int flags;
	unsigned int handle;
};

/**
 * User-desired user ptr import structure.
 *
 * @size: a pointer of size values to each buffer pointed by each user_ptr.
 * @user_ptr: points to user space address mmaped.
 * @handle: a pointer of gem handles.
 *	this variable would be set by gem framework of kernel side.
 *
 * this structure would be used to create new gem and the gem includes
 * physical address corresponding to user_ptr. the purpose of using this
 * structure is to create new gem object corresponding to user_ptr.
 */
struct drm_exynos_gem_userptr_imp {
	unsigned int size;
	uint64_t user_ptr;
	unsigned int handle;
};

/**
 * A structure for getting buffer offset.
 *
 * @handle: a pointer to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @offset: relatived offset value of the memory region allocated.
 *	- this value should be set by user.
 */
struct drm_exynos_gem_map_off {
	unsigned int handle;
	unsigned int pad;
	uint64_t offset;
};

/**
 * A structure for mapping buffer.
 *
 * @handle: a handle to gem object created.
 * @pad: just padding to be 64-bit aligned.
 * @size: memory size to be mapped.
 * @mapped: having user virtual address mmaped.
 *	- this variable would be filled by exynos gem module
 *	of kernel side with user virtual address which is allocated
 *	by do_mmap().
 */
struct drm_exynos_gem_mmap {
	unsigned int handle;
	unsigned int pad;
	uint64_t size;
	uint64_t mapped;
};

/**
 * A structure for ump.
 *
 * @gem_handle: a pointer to gem object created.
 * @secure_id: ump secure id and this value would be filled
 *		by kernel side.
 */
struct drm_exynos_gem_ump {
	unsigned int gem_handle;
	unsigned int secure_id;
};


/* temporary codes for legacy fimc and mfc drivers. */

/**
 * A structure for getting physical address corresponding to a gem handle.
 */
struct drm_exynos_gem_get_phy {
	unsigned int gem_handle;
	unsigned int pad;
	uint64_t size;
	uint64_t phy_addr;
};

/**
 * A structure for importing physical memory to a gem.
 */
struct drm_exynos_gem_phy_imp {
	uint64_t phy_addr;
	uint64_t size;
	unsigned int gem_handle;
	unsigned int pad;
};

/* indicate cache units. */
enum e_drm_exynos_gem_cache_sel {
	EXYNOS_DRM_L1_CACHE	= 1,
	EXYNOS_DRM_L2_CACHE	= 2,
	EXYNOS_DRM_ALL_CACHE	= 3
};

/* indicate cache operation types. */
enum e_drm_exynos_gem_cache_op {
	EXYNOS_DRM_CACHE_INV	= 4,
	EXYNOS_DRM_CACHE_CLN	= 8,
	EXYNOS_DRM_CACHE_FSH	= 0xC
};

/**
 * A structure for cache operation.
 *
 * @usr_addr: user space address.
 *	P.S. it SHOULD BE user space.
 * @size: buffer size for cache operation.
 * @flags: select cache unit and cache operation.
 */
struct drm_exynos_gem_cache_op {
	uint64_t usr_addr;
	unsigned int size;
	unsigned int flags;
};

struct drm_exynos_plane_set_zpos {
	__u32 plane_id;
	__s32 zpos;
};

#define DRM_EXYNOS_GEM_CREATE		0x00
#define DRM_EXYNOS_GEM_MAP_OFFSET	0x01
#define DRM_EXYNOS_GEM_MMAP		0x02
#define DRM_EXYNOS_GEM_USERPTR_IMP	0x03
#define DRM_EXYNOS_PLANE_SET_ZPOS	0x05

/* temporary ioctl command. */
#define DRM_EXYNOS_GEM_EXPORT_UMP	0x10
#define DRM_EXYNOS_GEM_CACHE_OP		0x12

#define DRM_EXYNOS_GEM_GET_PHY		0x13
#define DRM_EXYNOS_GEM_PHY_IMP		0x14

#define DRM_IOCTL_EXYNOS_GEM_CREATE		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_CREATE, struct drm_exynos_gem_create)

#define DRM_IOCTL_EXYNOS_GEM_MAP_OFFSET	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_MAP_OFFSET, struct drm_exynos_gem_map_off)

#define DRM_IOCTL_EXYNOS_GEM_MMAP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_MMAP, struct drm_exynos_gem_mmap)

#define DRM_IOCTL_EXYNOS_GEM_USERPTR_IMP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_USERPTR_IMP, struct drm_exynos_gem_userptr_imp)

#define DRM_IOCTL_EXYNOS_GEM_EXPORT_UMP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_EXPORT_UMP, struct drm_exynos_gem_ump)

#define DRM_IOCTL_EXYNOS_GEM_CACHE_OP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_CACHE_OP, struct drm_exynos_gem_cache_op)

/* temporary ioctl command. */
#define DRM_IOCTL_EXYNOS_GEM_GET_PHY	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_GET_PHY, struct drm_exynos_gem_get_phy)
#define DRM_IOCTL_EXYNOS_GEM_PHY_IMP	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_GEM_PHY_IMP, struct drm_exynos_gem_phy_imp)

#define DRM_IOCTL_EXYNOS_PLANE_SET_ZPOS	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_EXYNOS_PLANE_SET_ZPOS, struct drm_exynos_plane_set_zpos)

#endif