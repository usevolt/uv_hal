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


#ifndef CONFIG_UI_WINDOW_STYLES_COUNT
#define CONFIG_UI_WINDOW_STYLES_COUNT				1
#endif


/// @brief: GUI window style struct
typedef struct {
	color_t main_color;
	color_t frame_color;
	uint8_t frame_thickness;
} uv_window_style_st;
extern const uv_window_style_st uv_window_styles[CONFIG_UI_WINDOW_STYLES_COUNT];

#ifndef CONFIG_UI_WINDOW_COLOR_1
#define CONFIG_UI_WINDOW_COLOR_1 					0x888888
#endif

#ifndef CONFIG_UI_WINDOW_FRAME_COLOR_1
#define CONFIG_UI_WINDOW_FRAME_COLOR_1				0x222222
#endif

#ifndef CONFIG_UI_WINDOW_FRAME_THICKNESS_1
#define CONFIG_UI_WINDOW_FRAME_THICKNESS_1			3
#endif

#endif /* UV_HAL_INC_UI_UV_UI_STYLES_H_ */
