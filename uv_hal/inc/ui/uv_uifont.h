/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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


