/*
 * uv_ustyle.h
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UI_STYLES_H_
#define UV_HAL_INC_UI_UV_UI_STYLES_H_

/// @file: Style layout file. Defines the colors, shapes, etc of all GUI elements.
/// All layout styles can be overridden by providing the same symbol in the uv_hal_config.h file.


#include <uv_hal_config.h>
#include <uv_lcd.h>
<<<<<<< HEAD
#include "ui/uv_uifont.h"
=======

#if CONFIG_LCD
>>>>>>> 08f4e6180df4b48e162e74f624ab4d11a34674b4

#ifndef CONFIG_UI_WINDOW_STYLES_COUNT
#define CONFIG_UI_WINDOW_STYLES_COUNT				1
#endif
#ifndef CONFIG_UI_BUTTON_STYLES_COUNT
#define CONFIG_UI_BUTTON_STYLES_COUNT				1
#endif


/// @brief: GUI window style struct
typedef struct {
	color_t main_color;
	color_t frame_color;
	uint8_t frame_thickness;
} uv_uiwindow_style_st;
extern const uv_uiwindow_style_st uv_uiwindow_styles[CONFIG_UI_WINDOW_STYLES_COUNT];

#ifndef CONFIG_UI_WINDOW_COLOR_1
#define CONFIG_UI_WINDOW_COLOR_1 					C(0x888888)
#endif

#ifndef CONFIG_UI_WINDOW_FRAME_COLOR_1
#define CONFIG_UI_WINDOW_FRAME_COLOR_1				C(0x222222)
#endif

#ifndef CONFIG_UI_WINDOW_FRAME_THICKNESS_1
#define CONFIG_UI_WINDOW_FRAME_THICKNESS_1			3
#endif

<<<<<<< HEAD

typedef struct {
	color_t main_color;
	color_t frame_color;
	uint8_t frame_thickness;
	color_t text_color;
	const uv_font_st *text_font;
} uv_uibutton_style_st;
extern const uv_uibutton_style_st uv_uibutton_styles[CONFIG_UI_BUTTON_STYLES_COUNT];

#ifndef CONFIG_UI_BUTTON_COLOR_1
#define CONFIG_UI_BUTTON_COLOR_1				C(0xbbbbbb)
#endif
#ifndef CONFIG_UI_BUTTON_FRAME_COLOR_1
#define CONFIG_UI_BUTTON_FRAME_COLOR_1			C(0x333333)
#endif
#ifndef CONFIG_UI_BUTTON_FRAME_THICKNESS_1
#define CONFIG_UI_BUTTON_FRAME_THICKNESS_1		0x2
#endif
#ifndef CONFIG_UI_BUTTON_TEXT_COLOR_1
#define CONFIG_UI_BUTTON_TEXT_COLOR_1			C(0xeeeeee)
#endif
#ifndef CONFIG_UI_BUTTON_TEXT_FONT_1
#define CONFIG_UI_BUTTON_TEXT_FONT_1			font_5X12
=======
>>>>>>> 08f4e6180df4b48e162e74f624ab4d11a34674b4
#endif

#endif /* UV_HAL_INC_UI_UV_UI_STYLES_H_ */
