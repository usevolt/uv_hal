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

#ifndef CONFIG_UI_WINDOW_STYLES_COUNT
#define CONFIG_UI_WINDOW_STYLES_COUNT				1
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




#ifndef CONFIG_UI_BUTTON_STYLES_COUNT
#define CONFIG_UI_BUTTON_STYLES_COUNT				1
#endif

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
#endif



#ifndef CONFIG_UI_LIST_STYLES_COUNT
#define CONFIG_UI_LIST_STYLES_COUNT					1
#endif

typedef struct {
	color_t main_color;
	color_t highlight_color;
	color_t frame_color;
	uint8_t frame_thickness;
	color_t text_color;
	const uv_font_st *text_font;
} uv_uilist_style_st;
extern const uv_uilist_style_st uv_uilist_styles[CONFIG_UI_LIST_STYLES_COUNT];

#ifndef CONFIG_UI_LIST_COLOR_1
#define CONFIG_UI_LIST_COLOR_1					C(0x001100)
#endif
#ifndef CONFIG_UI_LIST_HIGHLIGHT_COLOR_1
#define CONFIG_UI_LIST_HIGHLIGHT_COLOR_1		C(0x005500)
#endif
#ifndef CONFIG_UI_LIST_FRAME_COLOR_1
#define CONFIG_UI_LIST_FRAME_COLOR_1			C(0x009900)
#endif
#ifndef CONFIG_UI_LIST_FRAME_THICKNESS_1
#define CONFIG_UI_LIST_FRAME_THICKNESS_1		1
#endif
#ifndef CONFIG_UI_LIST_TEXT_COLOR_1
#define CONFIG_UI_LIST_TEXT_COLOR_1				C(0xFFFFFF)
#endif
#ifndef CONFIG_UI_LIST_TEXT_FONT_1
#define CONFIG_UI_LIST_TEXT_FONT_1				font_5X12
#endif



#ifndef CONFIG_UI_SLIDER_STYLES_COUNT
#define CONFIG_UI_SLIDER_STYLES_COUNT			1
#endif

typedef struct {
	color_t bg_color;
	color_t handle_color;
	color_t frame_color;
	uint8_t frame_thickness;
	int16_t width;
	int16_t handle_width;
	color_t text_color;
	const uv_font_st *text_font;
} uv_uislider_style_st;
extern const uv_uislider_style_st uv_uislider_styles[CONFIG_UI_SLIDER_STYLES_COUNT];

#ifndef CONFIG_UI_SLIDER_BG_COLOR_1
#define CONFIG_UI_SLIDER_BG_COLOR_1				C(0x001100)
#endif
#ifndef CONFIG_UI_SLIDER_HANDLE_COLOR_1
#define CONFIG_UI_SLIDER_HANDLE_COLOR_1			C(0x005500)
#endif
#ifndef CONFIG_UI_SLIDER_FRAME_COLOR_1
#define CONFIG_UI_SLIDER_FRAME_COLOR_1			C(0x008800)
#endif
#ifndef CONFIG_UI_SLIDER_FRAME_THICKNESS_1
#define CONFIG_UI_SLIDER_FRAME_THICKNESS_1		1
#endif
#ifndef CONFIG_UI_SLIDER_WIDTH_1
#define CONFIG_UI_SLIDER_WIDTH_1				30
#endif
#ifndef CONFIG_UI_SLIDER_HANDLE_WIDTH_1
#define CONFIG_UI_SLIDER_HANDLE_WIDTH_1			50
#endif
#ifndef CONFIG_UI_SLIDER_TEXT_COLOR_1
#define CONFIG_UI_SLIDER_TEXT_COLOR_1			C(0xFFFFFF)
#endif
#ifndef CONFIG_UI_SLIDER_TEXT_FONT_1
#define CONFIG_UI_SLIDER_TEXT_FONT_1			font_5X12
#endif



#ifndef CONFIG_UI_KEYBOARD_STYLES_COUNT
#define CONFIG_UI_KEYBOARD_STYLES_COUNT			1
#endif

typedef struct {
	color_t bg_color;
	color_t key_color;
	color_t keytext_color;
	color_t frame_color;
	color_t text_color;
	color_t highlight_color;
	const uv_font_st *title_font;
	const uv_font_st *text_font;
} uv_uikeyboard_style_st;
extern const uv_uikeyboard_style_st uv_uikeyboard_styles[CONFIG_UI_KEYBOARD_STYLES_COUNT];

#ifndef CONFIG_UI_KEYBOARD_BG_COLOR_1
#define CONFIG_UI_KEYBOARD_BG_COLOR_1			C(0xFFFFFF)
#endif
#ifndef CONFIG_UI_KEYBOARD_KEY_COLOR_1
#define CONFIG_UI_KEYBOARD_KEY_COLOR_1			C(0x005500)
#endif
#ifndef CONFIG_UI_KEYBOARD_KEYTEXT_COLOR_1
#define CONFIG_UI_KEYBOARD_KEYTEXT_COLOR_1		C(0x000000)
#endif
#ifndef CONFIG_UI_KEYBOARD_FRAME_COLOR_1
#define CONFIG_UI_KEYBOARD_FRAME_COLOR_1		C(0x008800)
#endif
#ifndef CONFIG_UI_KEYBOARD_TEXT_COLOR_1
#define CONFIG_UI_KEYBOARD_TEXT_COLOR_1			C(0x000000)
#endif
#ifndef CONFIG_UI_KEYBOARD_HIGHLIGHT_COLOR_1
#define CONFIG_UI_KEYBOARD_HIGHLIGHT_COLOR_1	C(0x444444)
#endif
#ifndef CONFIG_UI_KEYBOARD_TITLE_FONT_1
#define CONFIG_UI_KEYBOARD_TITLE_FONT_1			font_5X12
#endif
#ifndef CONFIG_UI_KEYBOARD_TEXT_FONT_1
#define CONFIG_UI_KEYBOARD_TEXT_FONT_1			font_5X12
#endif


#endif

#endif /* UV_HAL_INC_UI_UV_UI_STYLES_H_ */
