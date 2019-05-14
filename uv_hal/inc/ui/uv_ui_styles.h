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
	color_t bg_c;
	color_t fg_c;
	color_t window_c;
	color_t display_c;
	uv_font_st *font;
	color_t text_color;

} uv_uistyle_st;
extern const uv_uistyle_st uv_uistyles[CONFIG_UI_STYLES_COUNT];

/* Example configuration:
#define CONFIG_UI_STYLE_BG_C_1					C(0xFF222222)
#define CONFIG_UI_STYLE_FG_C_1					C(0xFF666666)
#define CONFIG_UI_STYLE_FONT_1					font_5X12
#define CONFIG_UI_STYLE_WINDOW_C_1				C(0xFF000000)
#define CONFIG_UI_STYLE_DISPLAY_C_1				C(0xFF000000)
*/












#endif

#endif /* UV_HAL_INC_UI_UV_UI_STYLES_H_ */
