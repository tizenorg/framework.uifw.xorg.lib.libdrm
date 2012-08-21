/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2009-2010 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

/**
 * @file ump_ref_drv.h
 *
 * Reference driver extensions to the UMP user space API for allocating UMP memory
 */

#ifndef _UNIFIED_MEMORY_PROVIDER_REF_DRV_H_
#define _UNIFIED_MEMORY_PROVIDER_REF_DRV_H_

#include "ump.h"

#ifdef __cplusplus
extern "C" {
#endif

#define UMP_IOCTL_NR	0x90

typedef enum
{
	/* This enum must match with the IOCTL enum in ump_ioctl.h */
	UMP_REF_DRV_CONSTRAINT_NONE = 0,
	UMP_REF_DRV_CONSTRAINT_PHYSICALLY_LINEAR = 1,
	UMP_REF_DRV_CONSTRAINT_USE_CACHE = 128,
} ump_alloc_constraints;

typedef enum
{
	UMP_MSYNC_CLEAN = 0 ,
	UMP_MSYNC_CLEAN_AND_INVALIDATE = 1,
	UMP_MSYNC_READOUT_CACHE_ENABLED = 128,
} ump_cpu_msync_op;

struct ump_uk_dmabuf {
	void		*ctx;
	int		fd;
	size_t		size;
	uint32_t	ump_handle;
};

#define _UMP_IOC_DMABUF_IMPORT	8

#define UMP_IOC_DMABUF_IMPORT	_IOW(UMP_IOCTL_NR, _UMP_IOC_DMABUF_IMPORT,\
					struct ump_uk_dmabuf)

#ifdef __cplusplus
}
#endif

#endif /*_UNIFIED_MEMORY_PROVIDER_REF_DRV_H_ */
