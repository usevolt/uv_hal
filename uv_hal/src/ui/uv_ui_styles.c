/*
 * uv_ui_styles.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include "ui/uv_ui_styles.h"


#if CONFIG_LCD


#define STYLE(x)			,{\
		.active_bg_c = CAT(CONFIG_UI_STYLE_ACTIVE_BG_C_, INC(INC(x))), \
		.active_fg_c = CAT(CONFIG_UI_STYLE_ACTIVE_FG_C_, INC(INC(x))), \
		.active_frame_c = CAT(CONFIG_UI_STYLE_ACTIVE_FRAME_C_, INC(INC(x))), \
		.active_font_c = CAT(CONFIG_UI_STYLE_ACTIVE_FONT_C_, INC(INC(x))), \
		.inactive_bg_c = CAT(CONFIG_UI_STYLE_INACTIVE_BG_C_, INC(INC(x))), \
		.inactive_fg_c = CAT(CONFIG_UI_STYLE_INACTIVE_FG_C_, INC(INC(x))), \
		.inactive_frame_c = CAT(CONFIG_UI_STYLE_INACTIVE_FRAME_C_, INC(INC(x))), \
		.inactive_font_c = CAT(CONFIG_UI_STYLE_INACTIVE_FONT_C_, INC(INC(x))), \
		.font = & CAT(CONFIG_UI_STYLE_FONT_, INC(INC(x))), \
		.window_c = CAT(CONFIG_UI_STYLE_WINDOW_C_, INC(INC(x))) \
	}
const uv_uistyle_st uv_uistyles[CONFIG_UI_STYLES_COUNT] = {
		{
				.active_bg_c = CONFIG_UI_STYLE_ACTIVE_BG_C_1,
				.active_fg_c = CONFIG_UI_STYLE_ACTIVE_FG_C_1,
				.active_frame_c = CONFIG_UI_STYLE_ACTIVE_FRAME_C_1,
				.active_font_c = CONFIG_UI_STYLE_ACTIVE_FONT_C_1,
				.inactive_bg_c = CONFIG_UI_STYLE_INACTIVE_BG_C_1,
				.inactive_fg_c = CONFIG_UI_STYLE_INACTIVE_FG_C_1,
				.inactive_frame_c = CONFIG_UI_STYLE_INACTIVE_FRAME_C_1,
				.inactive_font_c = CONFIG_UI_STYLE_INACTIVE_FONT_C_1,
				.font = & CONFIG_UI_STYLE_FONT_1,
				.window_c = CONFIG_UI_STYLE_WINDOW_C_1
		}
		REPEAT(DEC(CONFIG_UI_STYLES_COUNT), STYLE)
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
