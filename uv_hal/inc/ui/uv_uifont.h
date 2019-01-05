/*
 * uv_uifont.h
 *
 *  Created on: Oct 17, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIFONT_H_
#define UV_HAL_INC_UI_UV_UIFONT_H_


#include <uv_hal_config.h>
#include <uv_utilities.h>

#if CONFIG_UI

#include "uv_ft81x.h"

typedef ft81x_font_st uv_font_st;

// non-antialiazed fonts not supported
//#define font4	ft81x_fonts[3]
//#define font5	ft81x_fonts[4]
//#define font6	ft81x_fonts[5]
//#define font7	ft81x_fonts[6]
//#define font8	ft81x_fonts[7]
//#define font9	ft81x_fonts[8]
//#define font10	ft81x_fonts[9]
#define font16	ft81x_fonts[0]
#define font20	ft81x_fonts[1]
#define font25	ft81x_fonts[2]
#define font28	ft81x_fonts[3]
#define font36	ft81x_fonts[4]
#define font49	ft81x_fonts[5]
#define font63	ft81x_fonts[6]
#define font83	ft81x_fonts[7]
#define font108	ft81x_fonts[8]




/// @brief: Vertical alignments
typedef enum {
	VALIGN_TOP = FT81X_VALIGN_TOP,
	VALIGN_CENTER = FT81X_VALIGN_CENTER,
} valignment_e;

/// @brief: Horizontal alignments
typedef enum {
	HALIGN_LEFT = FT81X_HALIGN_LEFT,
	HALIGN_CENTER = FT81X_HALIGN_CENTER,
	HALIGN_RIGHT = FT81X_HALIGN_RIGHT
} halignment_e;


/* Alignments */
typedef enum {
	ALIGN_TOP_LEFT 			= FT81X_ALIGN_LEFT_TOP,
	ALIGN_CENTER_LEFT 		= FT81X_ALIGN_LEFT_CENTER,
	ALIGN_TOP_CENTER 		= FT81X_ALIGN_CENTER_TOP,
	ALIGN_CENTER 			= FT81X_ALIGN_CENTER,
	ALIGN_TOP_RIGHT 		= FT81X_ALIGN_RIGHT_TOP,
	ALIGN_CENTER_RIGHT 		= FT81X_ALIGN_RIGHT_CENTER,
} alignment_e;


/// @brief: Returns the vertical alignment from the alignment *align*
static inline valignment_e uv_ui_get_valignment(alignment_e align) {
	return align & FT81X_VALIGN_MASK;
}

/// @brief: Returns the horizontal alignment from the alignment *align*
static inline halignment_e uv_ui_get_halignment(alignment_e align) {
	return align & FT81X_HALIGN_MASK;
}


#endif

#endif /* UV_HAL_INC_UI_UV_UIFONT_H_ */


