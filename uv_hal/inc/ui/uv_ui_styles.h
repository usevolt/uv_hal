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

#ifndef UV_HAL_INC_UI_UV_UI_STYLES_H_
#define UV_HAL_INC_UI_UV_UI_STYLES_H_

/// @file: Style layout file. Defines the colors, shapes, etc of all GUI elements.
/// All layout styles can be overridden by providing the same symbol in the uv_hal_config.h file.


#include <uv_hal_config.h>
#if CONFIG_LCD
#include <uv_lcd.h>
#elif CONFIG_FT81X
#include "uv_ft81x.h"
#endif
#include "ui/uv_uifont.h"

#if CONFIG_UI


#ifndef CONFIG_UI_STYLES_COUNT
#error "CONFIG_UI_STYLES_COUNT should declare the count of UI generic styles."
#endif

/// @brief: Generic style structure for UI
typedef struct {
	color_t active_bg_c;
	color_t active_fg_c;
	color_t active_frame_c;
	color_t active_font_c;
	color_t inactive_bg_c;
	color_t inactive_fg_c;
	color_t inactive_frame_c;
	color_t inactive_font_c;
	color_t window_c;
	color_t display_c;
	uv_font_st *font;
	color_t text_color;
	color_t shadow_c;
	color_t highlight_c;

} uv_uistyle_st;
extern const uv_uistyle_st uv_uistyles[CONFIG_UI_STYLES_COUNT];

/* Example configuration:
#define CONFIG_UI_STYLE_ACTIVE_BG_C_1			C(0xFF222222)
#define CONFIG_UI_STYLE_ACTIVE_FG_C_1			C(0xFF222222)
#define CONFIG_UI_STYLE_ACTIVE_FRAME_C_1		C(0xFF222222)
#define CONFIG_UI_STYLE_ACTIVE_FONT_C_1			C(0xFF222222)
#define CONFIG_UI_STYLE_INACTIVE_BG_C_1			C(0xFF222222)
#define CONFIG_UI_STYLE_INACTIVE_FG_C_1			C(0xFF222222)
#define CONFIG_UI_STYLE_INACTIVE_FRAME_C_1		C(0xFF222222)
#define CONFIG_UI_STYLE_INACTIVE_FONT_C_1		C(0xFF222222)
#define CONFIG_UI_STYLE_FONT_1					font_5X12
#define CONFIG_UI_STYLE_WINDOW_C_1				C(0xFF000000)
#define CONFIG_UI_STYLE_DISPLAY_C_1				C(0xFF000000)
*/












#endif

#endif /* UV_HAL_INC_UI_UV_UI_STYLES_H_ */
