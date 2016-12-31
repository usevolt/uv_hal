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
#include "ui/uv_uifont.h"

#if CONFIG_LCD


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
	const uv_font_st *font;
	color_t text_color;

} uv_uistyle_st;
extern const uv_uistyle_st uv_uistyles[CONFIG_UI_STYLES_COUNT];

/* Example configuration:
#define CONFIG_UI_STYLE_ACTIVE_BG_C_1			C(0x222222)
#define CONFIG_UI_STYLE_ACTIVE_FG_C_1			C(0x222222)
#define CONFIG_UI_STYLE_ACTIVE_FRAME_C_1		C(0x222222)
#define CONFIG_UI_STYLE_ACTIVE_FONT_C_1			C(0x222222)
#define CONFIG_UI_STYLE_INACTIVE_BG_C_1			C(0x222222)
#define CONFIG_UI_STYLE_INACTIVE_FG_C_1			C(0x222222)
#define CONFIG_UI_STYLE_INACTIVE_FRAME_C_1		C(0x222222)
#define CONFIG_UI_STYLE_INACTIVE_FONT_C_1		C(0x222222)
#define CONFIG_UI_STYLE_FONT_1					font_5X12
#define CONFIG_UI_STYLE_WINDOW_C_1				C(0x000000)
*/












#endif

#endif /* UV_HAL_INC_UI_UV_UI_STYLES_H_ */
