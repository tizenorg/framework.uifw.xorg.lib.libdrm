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
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _UAPI_SDP_DRM_H_
#define _UAPI_SDP_DRM_H_

#include <linux/videodev2.h> // fixme: how to remove denpendency ??

#include <drm/drm.h>

/* plane type */
enum sdp_drm_plane_type
{
	DRM_SDP_PLANE_OSDP,
	DRM_SDP_PLANE_GP,
	DRM_SDP_PLANE_SGP,
	DRM_SDP_PLANE_CURSOR1,
	DRM_SDP_PLANE_CURSOR2,
	DRM_SDP_PLANE_DP_MAIN,
	DRM_SDP_PLANE_DP_SUB,
	DRM_SDP_PLANE_DP_SUB2,
	DRM_SDP_PLANE_MAX
};

/* vblank type */
enum sdp_drm_vblank
{
	DRM_SDP_VBLANK_GFX = 0,
	DRM_SDP_VBLANK_DP = _DRM_VBLANK_SECONDARY,
};

/* flip type */
enum sdp_drm_flip {
	DRM_SDP_NO_FLIP,
	DRM_SDP_H_FLIP,
	DRM_SDP_V_FLIP,
	DRM_SDP_HV_FLIP,
	DRM_SDP_FLIP_MAX,
};

/* flip type */
enum sdp_drm_pivot {
	DRM_SDP_PIVOT_NORMAL,
	DRM_SDP_PIVOT_H_INV,
	DRM_SDP_PIVOT_V_INV,
	DRM_SDP_PIVOT_HV_INV,
	DRM_SDP_PIVOT_MAX,
};

enum sdp_drm_dp_plane_order {
	DRM_SDP_DP_PLANE_SMB,
	DRM_SDP_DP_PLANE_SBM,
	DRM_SDP_DP_PLANE_MSB,
	DRM_SDP_DP_PLANE_MBS,
	DRM_SDP_DP_PLANE_BSM,
	DRM_SDP_DP_PLANE_BMS,
	DRM_SDP_DP_PLANE_RESERVED,
	DRM_SDP_DP_PLANE_MB,
};

/* SDP GA */
enum sdp_drm_ga_bpp_mode{
	BPP8  = 1,
	BPP16 = 2,
	BPP32 = 4
};

enum sdp_drm_ga_op_type{
	SDP_GA_SOLID_FILL,
	SDP_GA_COPY, /* BITBLT*/
	SDP_GA_SCALE,
	SDP_GA_ROP /* BLEND */
};

enum sdp_drm_ga_color_mode{
	GFX_GA_FORMAT_32BPP_aRGB,
	GFX_GA_FORMAT_32BPP_RGBa,
	GFX_GA_FORMAT_16BPP,
	GFX_GA_FORMAT_8BPP,
	GFX_FORMAT_MAX
};

enum sdp_drm_ga_mode
{
	GA_BITBLT_MODE_NORMAL,
	GA_BITBLT_MODE_PREALPHA,
	GA_BITBLT_MODE_CA,
	GA_BITBLT_MODE_SRC,
	GA_BLOCKFILL_MODE,
	GA_MHP_ROP_CA_VALUE_MODE,
	GA_MHP_FILLED_ROP_CA_VALUE_MODE,
	GA_SCALE_MODE,
	GA_SCALEDROP_MODE,
	GA_YCBCR_PACK_MODE,/*spec out*/
	GA_MHP_ROP_CA_MODE,	/*spec out*/
	GA_MODE_END
};

struct sdp_drm_ga_src_info
{
	enum sdp_drm_ga_color_mode color_mode;
	uint32_t src_hbyte_size;
	uint32_t base_addr;
	uint32_t startx;
	uint32_t starty;
	uint32_t width;
	uint32_t height;
	enum sdp_drm_ga_bpp_mode bit_depth;
};

struct sdp_drm_ga_rect
{
	uint32_t x;
	uint32_t y;
	uint32_t w;
	uint32_t h;
};

enum sdp_drm_ga_mhp_rop
{
	S_ROP_COPY,
	S_ROP_ALPHA,
	S_ROP_TRANSPARENT,
	S_ROP_DVB_SRC,
	S_ROP_DVB_SRC_OVER,
	S_ROP_DVB_DST_OVER,
	S_ROP_DVB_SRC_IN,
	S_ROP_DVB_DST_IN,
	S_ROP_DVB_SRC_OUT,
	S_ROP_DVB_DST_OUT,
	S_ROP_OVERLAY,
	S_ROP_XOR,
	S_ROP_SRC_ATOP,
	S_ROP_DST_ATOP,
	S_ROP_DVB_CLEAR,
	S_ROP_8BPP_BLENDING
};

enum sdp_drm_ga_key_mode
{
	GA_KEYMODE_OFF,
	GA_KEYMODE_ROP_TRANSPARENT,
	GA_KEYMODE_8BPP
};

enum sdp_drm_ga_filled_rop_mode
{
	GA_SRC1_FILLCOLOR_SRC2_IMAGE,
	GA_SRC1_IMAGE_SRC2_FILLCOLOR
};

enum sdp_drm_ga_pre_mul_alpha_mode
{
	GA_PREMULTIPY_ALPHA_OFF_SHADOW=1,
	GA_PREMULTIPY_ALPHA_ON_SHADOW=3
};

struct sdp_drm_ga_pixmap_info {
	enum sdp_drm_ga_bpp_mode bit_depth;
	uint32_t		width;
	uint32_t		height;
};

struct sdp_drm_ga_solid_fill {
	enum sdp_drm_ga_color_mode color_mode;
	struct sdp_drm_ga_pixmap_info  pixmap;
	void			*paddr;
	uint32_t		handle;
	uint32_t		hbyte_size;
	uint32_t		h_start;
	uint32_t		v_start;
	uint32_t		h_size;
	uint32_t		v_size;
	uint32_t		color;
	uint32_t		stride;
};

struct sdp_drm_ga_scale
{
	enum sdp_drm_ga_color_mode color_mode;

	/* Scale Src */
	void		*src_paddr;
	uint32_t	src_handle;
	uint32_t	src_hbyte_size;;
	struct sdp_drm_ga_rect	src_rect;

	/* Scale Dst */
	void		*dst_paddr;
	uint32_t	dst_handle;
	uint32_t	dst_hbyte_size;
	struct sdp_drm_ga_rect	dst_rect;

	/* ROP */
	enum sdp_drm_ga_mhp_rop	rop_mode;
	enum sdp_drm_ga_pre_mul_alpha_mode pre_mul_alpha;
	uint32_t	rop_ca_value;
	uint32_t	src_key;
	uint32_t	rop_on_off;
};

struct sdp_drm_ga_bitblt
{
	enum sdp_drm_ga_color_mode color_mode;
	enum sdp_drm_ga_mode ga_mode;

	void *src1_paddr;
	uint32_t src1_handle;
	uint32_t src1_byte_size;
	struct sdp_drm_ga_rect src1_rect;

	void *dst_paddr;
	uint32_t dst_handle;
	uint32_t dst_byte_size;
	uint32_t dst_x;
	uint32_t dst_y;

	uint32_t ca_value;
};

struct sdp_drm_ga_rop
{
	enum sdp_drm_ga_mode ga_mode;
	enum sdp_drm_ga_color_mode color_mode;
	enum sdp_drm_ga_pre_mul_alpha_mode pre_mul_alpha;

	void		*src1_paddr;
	uint32_t	src1_handle;
	uint32_t	src1_byte_size;;
	struct sdp_drm_ga_rect	src1_rect;

	void		*dst_paddr;
	uint32_t	dst_handle;
	uint32_t	dst_byte_size;
	uint32_t	dst_x;
	uint32_t	dst_y;

	uint32_t	fill_color;

	union
	{
		struct mhp_const_t
		{
			enum sdp_drm_ga_mhp_rop rop_mode;
			uint32_t color_key;
			uint32_t ca_value;
		}mhp_const;
	}rop_mode;

	enum sdp_drm_ga_filled_rop_mode filled_rop_mode;
};

struct sdp_drm_ga_exec {
	enum sdp_drm_ga_op_type ga_op_type;

	/* One of the GA Operation*/
	union op{
		struct sdp_drm_ga_solid_fill solid_fill;
		struct sdp_drm_ga_bitblt bitblt;
		struct sdp_drm_ga_scale scale;
		struct sdp_drm_ga_rop rop;
		/* Add other GA Ops here */
	}ga_op;
};

/* memory type */
#define SDP_DRM_GEM_TYPE(x)	((x) & 0x3)
#define SDP_DRM_GEM_CONTIG	(0x0 << 0)
#define SDP_DRM_GEM_NONCONTIG	(0x1 << 0)
#define SDP_DRM_GEM_MP		(0x2 << 0)
#define SDP_DRM_GEM_HW		(0x3 << 0)

/* memory type : dp memory */
enum sdp_drm_gem_hw_mem_flag
{
	SDP_DRM_GEM_DP_FB_Y,
	SDP_DRM_GEM_DP_FB_C,
	SDP_DRM_GEM_DP_RM_CAPT_Y,
	SDP_DRM_GEM_DP_RM_CAPT_C,
	SDP_DRM_GEM_DP_MAIN_CAPT_Y,
	SDP_DRM_GEM_DP_MAIN_CAPT_C,
	SDP_DRM_GEM_DP_SUB_CAPT_Y,
	SDP_DRM_GEM_DP_SUB_CAPT_C,
	SDP_DRM_GEM_DP_MEM_MAX
};

#define DP_FB_MAIN				(0x0<<3)
#define DP_FB_SUB				(0x1<<3)

#define MAX_CRC_SIZE			30

/**
 * A structure for buffer creation
 *
 * @size: user-desired memory allocation size
 * @flags: for memory type
 * @handle: returned a handle to created gem object
 */
struct sdp_drm_gem_create {
	uint64_t	size;
	unsigned int	flags;
	unsigned int	handle;
};

/**
 * A structure for buffer creation
 *
 * @size: user-desired memory allocation size
 * @flags: for memory type
 * @handle: returned a handle to created gem object for HW
 */
struct sdp_drm_gem_hw_create {
	enum sdp_drm_gem_hw_mem_flag e_flag;
	unsigned int	userinfo;
	unsigned int	handle;
	uint64_t	ret_size;
};

/**
 * A structure for mapping buffer
 *
 * @handle: a handle to gem object created
 * @pad: for just padding 64bit aligned
 * @size: memory size to be mapped
 * @mapped: having user virtual address mmaped
 */
struct sdp_drm_gem_mmap {
	unsigned int	handle;
	unsigned int	pad;
	uint64_t	size;
	uint64_t	mapped;
};

enum sdp_drm_dp_out
{
	DRM_SDP_WINDOW,		/* screen plane */
	DRM_SDP_ENCODING,	/* encoding path */
	DRM_SDP_SCART_OUT_CVBS,	/* scart out - CVBS */
	DRM_SDP_SCART_OUT_BT656,/* scart out - BT656 */
	DRM_SDP_OUT_MAX
};

enum sdp_drm_3d_mode
{
	DRM_SDP_3D_2D,
	DRM_SDP_3D_FRAMEPACKING,
	DRM_SDP_3D_FRAMESEQ,
	DRM_SDP_3D_TOPBOTTOM,	
	DRM_SDP_3D_SIDEBYSIDE,
	DRM_SDP_3D_MAX
};

enum sdp_drm_3d_buf_mode
{
	DRM_SDP_3D_BUF_SINGLE,
	DRM_SDP_3D_BUF_DOUBLE,
	DRM_SDP_3D_BUF_MAX
};

enum sdp_drm_onoff_func {
	DRM_SDP_FUNC_SYNC_ONOFF,
	DRM_SDP_FUNC_MUTE_ONOFF,	
	DRM_SDP_FUNC_STILL_ONOFF,
	DRM_SDP_FUNC_GAMEMODE_ONOFF,
	DRM_SDP_FUNC_DP_PC_SHARPNESS_ONOFF,
	DRM_SDP_FUNC_MAX
};

struct sdp_drm_onoff_param
{
	uint32_t		plane_id;
	enum sdp_drm_onoff_func	set_func;

	uint32_t onoff_flag;
};

/* SDP DRM BUF. */
enum sdp_drm_dp_bufmode
{
	DRM_SDP_DP_BUF_Y_CBCR,
	DRM_SDP_DP_BUF_Y_CB_CR,
	DRM_SDP_DP_BUF_ARGB,
	DRM_SDP_DP_BUF_A_R_G_B,
	DRM_SDP_DP_BUF_MAX
};

struct sdp_drm_buf_addr{
	uint32_t addr[4];
	uint32_t size[4];
	enum sdp_drm_dp_bufmode buf_mod;
};

struct sdp_drm_buf
{
	struct sdp_drm_buf_addr addr_info;
	uint32_t u_width;
	uint32_t u_height;
};	

struct sdp_drm_dp_fb_addr{
	struct sdp_drm_buf_addr *info;
	uint32_t u_cur_color_fmt;
	uint32_t u_cur_width;
	uint32_t u_cur_height;
	uint32_t u_cur_y_bps;
	uint32_t u_cur_c_bps;
}; 

struct sdp_drm_dp_fb_c_size
{
	uint32_t u_color_format;
	uint32_t u_c_byteperline;
	uint32_t u_c_right_buf_offset;
};

struct sdp_drm_dp_fb_info
{
	uint32_t u_max_width;
	uint32_t u_max_height;
	uint32_t b_use_stereoscopic;	/* using flag 3D */
	uint32_t u_num_buffer;

	/* Y : byte per line*/
	uint32_t u_y_byteperline;
	uint32_t u_y_right_buf_offset;

	/* C : byte per line*/
	struct sdp_drm_dp_fb_c_size s_c_linesize[4];
};

struct sdp_drm_dp_fb_info_param
{
	uint32_t plane_id;
	struct sdp_drm_dp_fb_info s_ret_info;
};

struct sdp_drm_capture_info
{
	__u32 CpWrHSize;	/* capture  width */
	__u32 CpWrVSize;	/* capture height */
	__u32 CpWrYLineSize;	/* capture line size with padding 24 */
	__u32 CpWrCLineSize;	/* capture line size with padding 24 */
	__u32 tColorFormat;	/* captured data's format */
	__u32 u_is_flipped;	/* if flipped 1 otherwise 0 */
};

enum sdp_drm_capture_type
{
	DRM_SDP_CAPTURE_SCREEN,
	DRM_SDP_CAPTURE_VIDEO_ONLY,
	DRM_SDP_CAPTURE_VIDEO_0,
	DRM_SDP_CAPTURE_VIDEO_1,
	DRM_SDP_CAPTURE_MAX
};

struct sdp_drm_capture
{
	enum sdp_drm_capture_type	type;
	struct sdp_drm_capture_info	ret_data;
	__u32				reserved;
};

struct sdp_drm_qpi_out_crc
{
	uint32_t a_out_1_r[MAX_CRC_SIZE];
	uint32_t a_out_1_g[MAX_CRC_SIZE];
	uint32_t a_out_1_b[MAX_CRC_SIZE];
	uint32_t a_out_2_r[MAX_CRC_SIZE];
	uint32_t a_out_2_g[MAX_CRC_SIZE];
	uint32_t a_out_2_b[MAX_CRC_SIZE];
};

struct sdp_drm_qpi_dst_crc
{
	uint32_t a_luma_top[MAX_CRC_SIZE];
	uint32_t a_chrome_top[MAX_CRC_SIZE];
};

struct sdp_drm_qpi_crc
{
	uint32_t u_param_cnt;
	union{
		struct out_crc_param{
			uint32_t u_reserved;
		}out;

		struct dst_crc_param{
			uint32_t u_test_nrfcmode;	/* Test NRFC : Do Not[0], Do [1~] */
		}dst;
	}param;
	
	union{
		struct sdp_drm_qpi_out_crc s_out;
		struct sdp_drm_qpi_dst_crc s_dst;
	} rslt_crc;
};

struct sdp_drm_qpi_input_pattern
{
	enum sdp_drm_plane_type plane_type;	
	uint32_t onoff_flag;
	uint32_t pattern_type;
};

struct sdp_drm_qpi_incapt_clock_sel
{
	enum sdp_drm_plane_type plane_type;
	uint32_t clksel;
	uint32_t invert;
	uint32_t delay;
};

enum sdp_drm_resume
{
	DRM_SDP_RESUME_GAMMA,
	DRM_SDP_RESUME_MAX
};

struct sdp_drm_resume_param
{
	enum sdp_drm_resume e_data;
	uint32_t data_size;
	void *data;
};

/************************
* DRM ADDED PROPERTIES  *
*************************/
#define DRM_PROP_PLANE_TYPE_STR			"plane_type"
#define DRM_PROP_VBLANK_TYPE_STR		"vblank_type"
#define DRM_PROP_FLIP_STR			"flip_mode"
#define DRM_PROP_STEREOSCOPIC_STR		"stereo-scopic_mode"
#define DRM_PROP_GFX_ALPHA_STR			"gfxp_alpha"
#define DRM_PROP_GAMEMODE_STR			"game_mode"
#define DRM_PROP_DP_OUT_PATH_STR		"dp_out_path"
#define DRM_PROP_DP_FRAME_DOUBLE_STR		"dp_frame_doubling"
#define DRM_PROP_DP_PIVOT_STR			"dp_pivot"
#define DRM_PROP_CRTC_DP_PC_SHARPNESS_STR	"dp_pc_sharpness"
#define DRM_PROP_CRTC_DP_ORDER_STR		"dp_order"
#define DRM_PROP_GFX_ORDER_STR			"gfxp_order"

#define DRM_PROP_3D_BUF_MODE_SHIFT	8
#define DRM_PROP_3D_MODE_MASK		0xFF
#define DRM_PROP_3D_BUF_MODE_MASK	0xF00


/******************
* DRM ADDED IOCTL *
*******************/
#define DRM_SDP_GEM_CREATE		0x00
#define DRM_SDP_GEM_CREATE_HWMEM	0x01

#define DRM_SDP_SET_DP_SOURCE		0x11
#define DRM_SDP_CAPTURE			0x14
#define DRM_SDP_SET_ONOFF		0x15
#define DRM_SDP_GET_FB_INFO		0x16
#define DRM_SDP_SET_RESUME_DATA		0x17

#define DRM_SDP_GA_EXEC			0x40

#define DRM_SDP_QPI_GET_DST_CRC			0x50
#define DRM_SDP_QPI_GET_OUT_CRC			0x52
#define DRM_SDP_QPI_SET_INPUT_TEST_PATTERN	0x53
#define DRM_SDP_QPI_SET_INCAPT_CLOCK_SEL	0x54
#define DRM_SDP_QPI_SET_READY_GET_OUT_CRC	0x55
#define DRM_SDP_QPI_SET_GP_SYNCONOFF		0x56

#define DRM_IOCTL_SDP_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_GEM_CREATE,struct sdp_drm_gem_create)
#define DRM_IOCTL_SDP_GEM_CREATE_HWMEM	DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_GEM_CREATE_HWMEM,struct sdp_drm_gem_hw_create)

#define DRM_IOCTL_SDP_SET_DP_SOURCE	DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_SET_DP_SOURCE,struct v4l2_drm)
#define DRM_IOCTL_SDP_CAPTURE		DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_CAPTURE,struct sdp_drm_capture)
#define DRM_IOCTL_SDP_SET_ONOFF		DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_SET_ONOFF,struct sdp_drm_onoff_param)
#define DRM_IOCTL_SDP_GET_FB_INFO	DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_GET_FB_INFO,struct sdp_drm_dp_fb_info_param)
#define DRM_IOCTL_SDP_SET_RESUME_DATA	DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_SET_RESUME_DATA,struct sdp_drm_resume_param)

#define DRM_IOCTL_SDP_GA_EXEC		DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_GA_EXEC,struct sdp_drm_ga_exec)

#define DRM_IOCTL_SDP_QPI_GET_DST_CRC		 DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_QPI_GET_DST_CRC,struct sdp_drm_qpi_crc)
#define DRM_IOCTL_SDP_QPI_GET_OUT_CRC		 DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_QPI_GET_OUT_CRC,struct sdp_drm_qpi_crc)
#define DRM_IOCTL_SDP_QPI_SET_INPUT_TEST_PATTERN DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_QPI_SET_INPUT_TEST_PATTERN,struct sdp_drm_qpi_input_pattern)
#define DRM_IOCTL_SDP_QPI_SET_INCAPT_CLOCK_SEL	 DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_QPI_SET_INCAPT_CLOCK_SEL,struct sdp_drm_qpi_incapt_clock_sel)
#define DRM_IOCTL_SDP_QPI_SET_READY_GET_OUT_CRC	 DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_QPI_SET_READY_GET_OUT_CRC,uint32_t)
#define DRM_IOCTL_SDP_QPI_SET_GP_SYNCONOFF 	 DRM_IOWR(DRM_COMMAND_BASE+DRM_SDP_QPI_SET_GP_SYNCONOFF,uint32_t)

#endif /* _UAPI_SDP_DRM_H_ */

