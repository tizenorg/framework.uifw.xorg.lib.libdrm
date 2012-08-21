#ifndef _G2D_H_
#define _G2D_H_

#include "g2d_reg.h"

typedef enum {
	G2D_SELECT_MODE_NORMAL = (0 << 0),
	G2D_SELECT_MODE_FGCOLOR = (1 << 0),
	G2D_SELECT_MODE_BGCOLOR = (2 << 0),
	G2D_SELECT_MODE_MAX = (3 << 0),
} G2dSelectMode;

typedef enum {
	/* COLOR FORMAT */
	G2D_COLOR_FMT_XRGB8888,
	G2D_COLOR_FMT_ARGB8888,
	G2D_COLOR_FMT_RGB565,
	G2D_COLOR_FMT_XRGB1555,
	G2D_COLOR_FMT_ARGB1555,
	G2D_COLOR_FMT_XRGB4444,
	G2D_COLOR_FMT_ARGB4444,
	G2D_COLOR_FMT_PRGB888,
	G2D_COLOR_FMT_YCbCr444,
	G2D_COLOR_FMT_YCbCr422,
	G2D_COLOR_FMT_YCbCr420 = 10,
	G2D_COLOR_FMT_A8,                       /* alpha 8bit */
	G2D_COLOR_FMT_L8,                       /* Luminance 8bit: gray color */
	G2D_COLOR_FMT_A1,                       /* alpha 1bit */
	G2D_COLOR_FMT_A4,                       /* alpha 4bit */
	G2D_COLOR_FMT_MASK = (15 << 0),		/* VER4.1 */

	/* COLOR ORDER */
	G2D_ORDER_AXRGB = (0 << 4),			/* VER4.1 */
	G2D_ORDER_RGBAX = (1 << 4),			/* VER4.1 */
	G2D_ORDER_AXBGR = (2 << 4),			/* VER4.1 */
	G2D_ORDER_BGRAX = (3 << 4),			/* VER4.1 */
	G2D_ORDER_MASK = (3 << 4),			/* VER4.1 */

	/* Number of YCbCr plane */
	G2D_YCbCr_1PLANE = (0 << 8),			/* VER4.1 */
	G2D_YCbCr_2PLANE = (1 << 8),			/* VER4.1 */
	G2D_YCbCr_PLANE_MASK = (3 << 8),		/* VER4.1 */

	/* Order in YCbCr */
	G2D_YCbCr_ORDER_CrY1CbY0 = (0 << 12),		/* VER4.1 */
	G2D_YCbCr_ORDER_CbY1CrY0 = (1 << 12),		/* VER4.1 */
	G2D_YCbCr_ORDER_Y1CrY0Cb = (2 << 12),		/* VER4.1 */
	G2D_YCbCr_ORDER_Y1CbY0Cr = (3 << 12),		/* VER4.1 */
	G2D_YCbCr_ORDER_MASK = (3 < 12),		/* VER4.1 */

	/* CSC */
	G2D_CSC_601 = (0 << 16),			/* VER4.1 */
	G2D_CSC_709 = (1 << 16),			/* VER4.1 */
	G2D_CSC_MASK = (1 << 16),			/* VER4.1 */

	/* Valid value range of YCbCr */
	G2D_YCbCr_RANGE_NARROW = (0 << 17),		/* VER4.1 */
	G2D_YCbCr_RANGE_WIDE = (1 << 17),		/* VER4.1 */
	G2D_YCbCr_RANGE_MASK= (1 << 17),		/* VER4.1 */

	G2D_COLOR_MODE_MASK = 0xFFFFFFFF
} G2dColorMode;

#endif /* _G2D_H_ */
