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


#ifndef UV_UI_COMMON_H_
#define UV_UI_COMMON_H_

#include "uv_utilities.h"


/// @file: Defines the GUI drawing interface. These functions need to be implemented
/// on different targets

/// @brief: Wrapper for font data for UI library
typedef struct {
	uint16_t char_height;
	// defines the font index. Index 26-34 are anti-aliased fonts which are supported by this library
	uint8_t index;
	// handle defines the bitmap handle to be used for this font
	uint8_t handle;
} ui_font_st;
typedef ui_font_st uv_font_st;


/// @brief: FT81x library supports only anti-aliased fonts with index 26 - 34
#define UI_MAX_FONT_COUNT				9
extern ui_font_st ui_fonts[UI_MAX_FONT_COUNT];



typedef enum {
	UI_VALIGN_TOP = 0,
	UI_VALIGN_CENTER = 0x400
} ui_valign_e;
#define UI_VALIGN_MASK 		0x400

typedef enum {
	UI_HALIGN_LEFT = 0,
	UI_HALIGN_CENTER = 0x200,
	UI_HALIGN_RIGHT = 0x800
} ui_halign_e;
#define UI_HALIGN_MASK 		(0x200 | 0x800)


typedef enum {
	UI_ALIGN_LEFT_TOP = UI_HALIGN_LEFT | UI_VALIGN_TOP,
	UI_ALIGN_LEFT_CENTER = UI_HALIGN_LEFT | UI_VALIGN_CENTER,
	UI_ALIGN_RIGHT_TOP = UI_HALIGN_RIGHT | UI_VALIGN_TOP,
	UI_ALIGN_RIGHT_CENTER = UI_HALIGN_RIGHT | UI_VALIGN_CENTER,
	UI_ALIGN_CENTER = UI_HALIGN_CENTER | UI_VALIGN_CENTER,
	UI_ALIGN_CENTER_TOP = UI_HALIGN_CENTER | UI_VALIGN_TOP
} ui_align_e;


typedef enum {
	COLOR_MODE_RGB = 0,
	COLOR_MODE_GRAYSCALE
} ui_color_modes_e;



/// @brief: Width of the LCD in pixels
#define LCD_W_PX		CONFIG_FT81X_HSIZE


/// @brief: Height of the LCD in pixels
#define LCD_H_PX		CONFIG_FT81X_VSIZE


/// @brief: Converts from relative 0.0f - 1.0f width to actual pixel width
#define LCD_W(rel_w)	(LCD_W_PX * (rel_w))
#define LCD_WPPT(w_ppt) (LCD_W_PX * w_ppt / 1000)


/// @brief: Converts from relative 0.0f - 1.0f height to actual pixel height
#define LCD_H(rel_h)	(LCD_H_PX * (rel_h))
#define LCD_HPPT(h_ppt)	(LCD_H_PX * h_ppt / 1000)



/// @brief: Color typedef
typedef struct {
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;
} color_st;
typedef uint32_t color_t;
// Macro which should be used when colors are assigned. This is to keep
// UI libraries compatible with different color spaces than ARGB8888.
// Give the color as C(0xAARRGGBB)
#define C(x)		(x)

/// @brief: Returns a color which is brightened by removing *value* amount of 8-bit color
/// from all R, G and B color channels
color_t uv_uic_brighten(color_t c, int8_t value);


/// @brief: Returns a color by adding *value* amount of alpha to the given color *c*
color_t uv_uic_alpha(color_t c, int8_t value);

/// @brief: Converts *c* to grayscale and returns it
color_t uv_uic_grayscale(color_t c);

/// @brief: Returns a color which is linearily interpolated between colors ca and cb.
/// t = 0 returns ca, t = 1000 returns cb. No boundary checks are done in the calculations.
color_t uv_uic_lerpi(int32_t t, color_t ca, color_t cb);

/// @brief: Copies the alpha channel from *dest* color to *src* color
static inline void uv_uic_copy_alpha(color_t *dest, color_t *src) {
	((color_st*) dest)->a = ((color_st *)src)->a;
}

/// @brief: Struct for individual object's bounding box.
typedef struct {
	/// @brief: local left-most x coordinate relative to the parent
	int16_t x;
	/// @brief: Local top-most y-coordinate relative to the parent
	int16_t y;
	union {
		/// @brief: Width, growing to right
		int16_t width;
		int16_t w;
	};
	union {
		/// @brief: Height, growing to bottom
		int16_t height;
		int16_t h;
	};
} uv_bounding_box_st;

static inline bool uv_bb_is_null(uv_bounding_box_st *bb) {
	return bb->x == 0 &&
			bb->y == 0 &&
			bb->width == 0 &&
			bb->height == 0;
}



#endif /* UV_UI_COMMON_H_ */
