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


#if 0
/** Input port definition */
enum e_drm_sdp_dp_inputport 
{
	/*! Input : MFD Decoder-0 */	
	DRM_SDP_DP_INPORT_MFD0,
	/*! Input : MFD Decoder-1 */	
	DRM_SDP_DP_INPORT_MFD1,	
	/*! Input : DSP */// DP_081106_4
	DRM_SDP_DP_INPORT_DSP,
	/*! Input : Picture image(JPEG, etc) */	
	DRM_SDP_DP_INPORT_PIC,	
	/*! Input : Input Capture-0 */	
	DRM_SDP_DP_INPORT_INCAPT0,
	/*! Input : Input Capture-1 */	
	DRM_SDP_DP_INPORT_INCAPT1,
	/*! Input : Test Pattern */ 
	DRM_SDP_DP_INPORT_TP,
	/*! Input : None */    
	DRM_SDP_DP_INPORT_NONE,    
};

/** Input type definition */
enum e_drm_sdp_dp_inputtype
{	
	DRM_SDP_DP_INTYPE_MPEG,			/**< Input type : MPEG */
	DRM_SDP_DP_INTYPE_MPEG2,		/**< Input type : MPEG2 */
	DRM_SDP_DP_INTYPE_MPEG4,		/**< Input type : MPEG4 */
	DRM_SDP_DP_INTYPE_H263,			/**< Input type : H263 */
	DRM_SDP_DP_INTYPE_H264,			/**< Input type : H264 */
	DRM_SDP_DP_INTYPE_VC1,			/**< Input type : VC1 */
	DRM_SDP_DP_INTYPE_DIVX,			/**< Input type : Divx */
	DRM_SDP_DP_INTYPE_MVC,			/**< Input type : Divx */
	DRM_SDP_DP_INTYPE_DUALCODEC,		/**< Input type : DUAL_CODEC */

	//Picture Source
	DRM_SDP_DP_INTYPE_SWDECPIC,		/**< Input type : Picture(JPEG/PNG) by SW decoder */
	DRM_SDP_DP_INTYPE_HWDECPIC,		/**< Input type : Picture(JPEG/PNG) by HW decoder */	
	DRM_SDP_DP_INTYPE_FLASH,		/**< Input type : T-Lib */
	DRM_SDP_DP_INTYPE_JPEG_VDECREAD,		/* vdec 으로 read 하는 path (실제로 pic 인 경우 inptype 안봄, 실제 시나리오 정립 이후 type 변경 */ 
  	DRM_SDP_DP_INTYPE_GRAPHIC_VDECREAD,	/* vdec 으로 read 하는 path (실제로 pic 인 경우 inptype 안봄, 실제 시나리오 정립 이후 type 변경 */ 
	DRM_SDP_DP_INTYPE_WEBCAM_VDECREAD,		/* vdec 으로 read 하는 path (실제로 pic 인 경우 inptype 안봄, 실제 시나리오 정립 이후 type 변경 */ 
  	
 
	DRM_SDP_DP_INTYPE_INCAPT,		/**< Input type : Input Capture */
	//Internal Chip Source	  
	DRM_SDP_DP_INTYPE_INAV,			/**< Input type : Internal AV */
	DRM_SDP_DP_INTYPE_INCOMP,		/**< Input type : Internal Component */
	DRM_SDP_DP_INTYPE_INRGB,		/**< Input type : Internal RGB */
	DRM_SDP_DP_INTYPE_INDVI,		/**< Input type : Internal DVI */
	DRM_SDP_DP_INTYPE_INHDMI,		/**< Input type : Internal HDMI */

	//External Chip Source	
	DRM_SDP_DP_INTYPE_EXTAV,		/**< Input type : External AV */
	DRM_SDP_DP_INTYPE_EXTCOMP,		/**< Input type : External Component */
	DRM_SDP_DP_INTYPE_EXTRGB,		/**< Input type : External RGB */
	DRM_SDP_DP_INTYPE_EXTDVI,		/**< Input type : External DVI */
	DRM_SDP_DP_INTYPE_EXTHDMI,		/**< Input type : External HDMI */

	//Test Pattern	
	DRM_SDP_DP_INTYPE_TEST,			/**< Input type : Test */

	DRM_SDP_DP_INTYPE_FEEDBACK,
	DRM_SDP_DP_INTYPE_LVDSRX,
	//Etc	
	DRM_SDP_DP_INTYPE_VDEC,			/**< Input type : VDEC */
	DRM_SDP_DP_INTYPE_UNDEFINED,	/**< Input type : Undefined */
	DRM_SDP_DP_INTYPE_MAX,			/**< Number of input type */   
};
#endif

/* SDP custom plane id definitions. */
enum e_drm_sdp_plane_type {
    DRM_SDP_PLANE_ID_V0 = 8, ///< video plane with scalar #0 (picture enhancer)
    DRM_SDP_PLANE_ID_V1 = 12, ///< video plane with scalar #1
    DRM_SDP_PLANE_ID_G0 = 13, ///< graphic plane 0
    DRM_SDP_PLANE_ID_G1 = 14, ///< graphic plane 1
    DRM_SDP_PLANE_ID_G2 = 15  ///< graphic plane 2
};

/** Input port definition */
enum e_drm_sdp_dp_inputport {
	DRM_SDP_DP_INPORT_DTV,
	DRM_SDP_DP_INPORT_MM,	
	DRM_SDP_DP_INPORT_ATV,
	DRM_SDP_DP_INPORT_AV,
	DRM_SDP_DP_INPORT_COMPONENT,
	DRM_SDP_DP_INPORT_RGB,
	DRM_SDP_DP_INPORT_DVI,	
	DRM_SDP_DP_INPORT_HDMI,	
	DRM_SDP_DP_INPORT_CLONE,		
	DRM_SDP_DP_INPORT_NONE    
};

struct sdp_drm_dec_info {
	__u32	dp_mfd_Hor_Resolution;					/**< Source horizontal size */
	__u32	dp_mfd_Ver_Resolution;					/**< Source vertical size */
	__u32	dp_mfd_AspectRatio;						/**< Source aspect ratio information 
												* - 0 : fobid
												* - 1 : 1/1
												* - 2 : 3/4
												* - 3 : 9/16
												* - 4 : 1/2.21
												* - 5 : reserved
												*/
	__u32	dp_mfd_FrameRate;						/**< Source frame rate 
												* - 1 : 23.975
												* - 2 : 24
												* - 3 : 25
												* - 4 : 29.97
												* - 5 : 30
												* - 6 : 50
												* - 7 : 59.94
												* - 8 : 60										 
												*/
	__u32	dp_mfd_ScanType;						/**< Source scan type */
	
	__u32	dp_mfd_dis_ptr;    
	__u32	dp_mfd_show_frame;    
	__u32	dp_mfd_chip_id;    	
	__u32	dp_mfd_inst;    		

};

struct sdp_drm_dp_src_info{
	__u32       					dp_plane;	//main/sub
	enum e_drm_sdp_dp_inputport	dp_input_port;
//	enum e_drm_sdp_dp_inputtype	dp_input_type;    
	struct sdp_drm_dec_info		dp_dec_info;
	__u32						unmute_flag;	
	__u32						reserved;
};

enum e_drm_sdp_zorder {
    DRM_SDP_ZORDER_TOP_VIDEO      = 0, ///< top video, bottom graphic
    DRM_SDP_ZORDER_BOTTOM_GRAPHIC, ///< top video, bottom graphic
    DRM_SDP_ZORDER_BOTTOM_VIDEO,	 ///< bottom video, top graphic
    DRM_SDP_ZORDER_TOP_GRAPHIC , 	///< bottom video, top graphic
    DRM_SDP_ZORDER_S_M_B , 		///< Sub/Main/Bg (Default Window Order)
    DRM_SDP_ZORDER_S_B_M , 
    DRM_SDP_ZORDER_M_S_B , 
    DRM_SDP_ZORDER_M_B_S , 
    DRM_SDP_ZORDER_B_S_M ,
    DRM_SDP_ZORDER_B_M_S        
};


struct sdp_drm_zorder {
	__u32       				dp_plane;	//main/sub
	enum e_drm_sdp_zorder 	dp_zorder;
	__u32					reserved;
};

/** Encoder type definition */
enum e_drm_sdp_enc_type 
{
	DRM_SDP_ENC_CLONE_VIEW,
	DRM_SDP_ENC_DUAL_VIEW,
	DRM_SDP_ENC_MAX
};

struct sdp_drm_enc_info
{
	enum e_drm_sdp_enc_type	enc_type;		// clone or dual view
	__u32      				enc_scl_h;		//target height size
	__u32      				enc_scl_v;		//target vertical size
	__u32     					reserved;
};

struct sdp_drm_dp_capt_info
{
	__u32 ChipId;		// not used now...
	__u32 CpWrHSize;	//capture  width
	__u32 CpWrVSize;	//capture height
	__u32 CpWrYBase;	
	__u32 CpWrCBase;
	__u32 CpWrLineSize;	// capture line size with padding 24
	__u32 tColorFormat;
};

enum e_drm_sdp_capt_type	
{
	DRM_SDP_CAPT_RM,
	DRM_SDP_CAPT_PN,
	DRM_SDP_CAPT_MAX
};

struct  sdp_drm_capture_info
{
	__u32       					dp_plane;	//main/sub
	enum e_drm_sdp_capt_type		dp_capt_type;	
	struct sdp_drm_dp_capt_info	dp_capt_info;
	__u32     						reserved;
};

/* memory type */
#define SDP_DRM_GEM_TYPE(x)		((x) & 0x3)
#define SDP_DRM_GEM_CONTIG		(0x0 << 0)
#define SDP_DRM_GEM_NONCONTIG		(0x1 << 0)
#define SDP_DRM_GEM_MP			(0x2 << 0)

#define DRM_SDP_GEM_CREATE			0x00
//#define DRM_SDP_GEM_MMAP				0x01
#define DRM_SDP_SET_DP_INFO			0x01
#define DRM_SDP_SET_ZORDER			0x02
#define DRM_SDP_SET_ENC_INFO			0x03
#define DRM_SDP_GET_CAPTURE_INFO		0x04

#define DRM_IOCTL_SDP_GEM_CREATE	DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SDP_GEM_CREATE, struct sdp_drm_gem_create)
//#define DRM_IOCTL_SDP_GEM_MMAP	DRM_IOWR(DRM_COMMAND_BASE + \
//		DRM_SDP_GEM_MMAP, struct sdp_drm_gem_mmap)
#define DRM_IOCTL_SDP_SET_DP_INFO		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SDP_SET_DP_INFO, struct sdp_drm_dp_src_info)
#define DRM_IOCTL_SDP_SET_ZORDER		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SDP_SET_ZORDER, struct  sdp_drm_zorder)
#define DRM_IOCTL_SDP_SET_ENC_INFO		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SDP_SET_ENC_INFO, struct  sdp_drm_enc_info)
#define DRM_IOCTL_SDP_GET_CAPTURE_INFO		DRM_IOWR(DRM_COMMAND_BASE + \
		DRM_SDP_GET_CAPTURE_INFO, struct  sdp_drm_capture_info)
		
#endif /* _UAPI_SDP_DRM_H_ */
