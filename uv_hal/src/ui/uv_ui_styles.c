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
		.text_color = CAT(CONFIG_UI_STYLE_TEXT_COLOR_, INC(INC(x))), \
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
				.text_color = CONFIG_UI_STYLE_TEXT_COLOR_1,
				.window_c = CONFIG_UI_STYLE_WINDOW_C_1
		}
		REPEAT(DEC(CONFIG_UI_STYLES_COUNT), STYLE)
};









#endif
