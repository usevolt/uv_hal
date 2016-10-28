/*
 * uv_ui_styles.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include "ui/uv_ui_styles.h"


#if CONFIG_LCD

#define WINDOW_STYLE(x)		,{\
	.main_color= CAT(CONFIG_UI_WINDOW_COLOR_, INC(INC(x))), \
				.frame_color = CAT(CONFIG_UI_WINDOW_FRAME_COLOR_, INC(INC(x))), \
				.frame_thickness = CAT(CONFIG_UI_WINDOW_FRAME_THICKNESS_, INC(INC(x))) \
		}

const uv_uiwindow_style_st uv_uiwindow_styles[CONFIG_UI_WINDOW_STYLES_COUNT] = {
		{
				.main_color= CONFIG_UI_WINDOW_COLOR_1,
				.frame_color = CONFIG_UI_WINDOW_FRAME_COLOR_1,
				.frame_thickness = CONFIG_UI_WINDOW_FRAME_THICKNESS_1
		}
		REPEAT(DEC(CONFIG_UI_WINDOW_STYLES_COUNT), WINDOW_STYLE)

};


#define BUTTON_STYLE(x)		,{\
			.main_color = CAT(CONFIG_UI_BUTTON_COLOR_, INC(INC(x))), \
			.frame_color = CAT(CONFIG_UI_BUTTON_FRAME_COLOR_, INC(INC(x))), \
			.frame_thickness = CAT(CONFIG_UI_BUTTON_FRAME_THICKNESS_, INC(INC(x))), \
			.text_color = CAT(CONFIG_UI_BUTTON_TEXT_COLOR_, INC(INC(x))), \
			.text_font = & CAT(CONFIG_UI_BUTTON_TEXT_FONT_, INC(INC(x))) \
		}
const uv_uibutton_style_st uv_uibutton_styles[CONFIG_UI_BUTTON_STYLES_COUNT] = {
		{
				.main_color = CONFIG_UI_BUTTON_COLOR_1,
				.frame_color = CONFIG_UI_BUTTON_FRAME_COLOR_1,
				.frame_thickness = CONFIG_UI_BUTTON_FRAME_THICKNESS_1,
				.text_color = CONFIG_UI_BUTTON_TEXT_COLOR_1,
				.text_font = & CONFIG_UI_BUTTON_TEXT_FONT_1
		}
		REPEAT(DEC(CONFIG_UI_BUTTON_STYLES_COUNT), BUTTON_STYLE)
};




#define LIST_STYLE(x)			,{\
		.main_color = CAT(CONFIG_UI_LIST_COLOR_, INC(INC(x))), \
		.highlight_color = CAT(CONFIG_UI_LIST_HIGHLIGHT_COLOR_, INC(INC(x))), \
		.frame_color = CAT(CONFIG_UI_LIST_FRAME_COLOR_, INC(INC(x))), \
		.frame_thickness = CAT(CONFIG_UI_LIST_FRAME_THICKNESS_, INC(INC(x))), \
		.text_color = CAT(CONFIG_UI_LIST_TEXT_COLOR_, INC(INC(x))), \
		.text_font = & CAT(CONFIG_UI_LIST_TEXT_FONT_, INC(INC(x))) \
	}
const uv_uilist_style_st uv_uilist_styles[CONFIG_UI_LIST_STYLES_COUNT] = {
		{
				.main_color = CONFIG_UI_LIST_COLOR_1,
				.highlight_color = CONFIG_UI_LIST_HIGHLIGHT_COLOR_1,
				.frame_color = CONFIG_UI_LIST_FRAME_COLOR_1,
				.frame_thickness = CONFIG_UI_LIST_FRAME_THICKNESS_1,
				.text_color = CONFIG_UI_LIST_TEXT_COLOR_1,
				.text_font = & CONFIG_UI_LIST_TEXT_FONT_1
		}
		REPEAT(DEC(CONFIG_UI_LIST_STYLES_COUNT), LIST_STYLE)
};



#define SLIDER_STYLE(x)			,{\
		.bg_color = CAT(CONFIG_UI_SLIDER_BG_COLOR_, INC(INC(x))), \
		.handle_color = CAT(CONFIG_UI_SLIDER_HANDLE_COLOR_, INC(INC(x))), \
		.frame_color = CAT(CONFIG_UI_SLIDER_FRAME_COLOR_, INC(INC(x))), \
		.frame_thickness = CAT(CONFIG_UI_SLIDER_FRAME_THICKNESS_, INC(INC(x))), \
		.width = CAT(CONFIG_UI_SLIDER_WIDTH_, INC(INC(x))), \
		.handle_width = CAT(CONFIG_UI_SLIDER_WIDTH_, INC(INC(x))), \
		.text_color = CAT(CONFIG_UI_SLIDER_TEXT_COLOR_, INC(INC(x))), \
		.text_font = & CAT(CONFIG_UI_SLIDER_TEXT_FONT_, INC(INC(x))) \
	}
const uv_uislider_style_st uv_uislider_styles[CONFIG_UI_SLIDER_STYLES_COUNT] = {
		{
				.bg_color = CONFIG_UI_SLIDER_BG_COLOR_1,
				.handle_color = CONFIG_UI_SLIDER_HANDLE_COLOR_1,
				.frame_color = CONFIG_UI_SLIDER_FRAME_COLOR_1,
				.frame_thickness = CONFIG_UI_SLIDER_FRAME_THICKNESS_1,
				.width = CONFIG_UI_SLIDER_WIDTH_1,
				.handle_width = CONFIG_UI_SLIDER_HANDLE_WIDTH_1,
				.text_color = CONFIG_UI_SLIDER_TEXT_COLOR_1,
				.text_font = & CONFIG_UI_SLIDER_TEXT_FONT_1
		}
		REPEAT(DEC(CONFIG_UI_SLIDER_STYLES_COUNT), SLIDER_STYLE)
};



#define KEYBOARD_STYLE(x)			,{\
		.bg_color = CAT(CONFIG_UI_KEYBOARD_BG_COLOR_, INC(INC(x))), \
		.key_color = CAT(CONFIG_UI_KEYBOARD_KEY_COLOR_, INC(INC(x))), \
		.keytext_color = CAT(CONFIG_UI_KEYBOARD_KEYTEXT_COLOR_, INC(INC(x))), \
		.frame_color = CAT(CONFIG_UI_KEYBOARD_FRAME_COLOR_, INC(INC(x))), \
		.text_color = CAT(CONFIG_UI_KEYBOARD_TEXT_COLOR_, INC(INC(x))), \
		.highlight_color = CAT(CONFIG_UI_KEYBOARD_HIGHLIGHT_COLOR_, INC(INC(x))), \
		.title_font = & CAT(CONFIG_UI_KEYBOARD_TITLE_FONT_, INC(INC(x))), \
		.text_font = & CAT(CONFIG_UI_KEYBOARD_TEXT_FONT_, INC(INC(x))) \
	}
const uv_uikeyboard_style_st uv_uikeyboard_styles[CONFIG_UI_KEYBOARD_STYLES_COUNT] = {
		{
				.bg_color = CONFIG_UI_KEYBOARD_BG_COLOR_1,
				.key_color = CONFIG_UI_KEYBOARD_KEY_COLOR_1,
				.keytext_color = CONFIG_UI_KEYBOARD_KEYTEXT_COLOR_1,
				.frame_color = CONFIG_UI_KEYBOARD_FRAME_COLOR_1,
				.text_color = CONFIG_UI_KEYBOARD_TEXT_COLOR_1,
				.highlight_color = CONFIG_UI_KEYBOARD_HIGHLIGHT_COLOR_1,
				.title_font = & CONFIG_UI_KEYBOARD_TITLE_FONT_1,
				.text_font = & CONFIG_UI_KEYBOARD_TEXT_FONT_1
		}
		REPEAT(DEC(CONFIG_UI_KEYBOARD_STYLES_COUNT), KEYBOARD_STYLE)
};


#endif
