/*
 * uv_ui_styles.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include "ui/uv_ui_styles.h"


#if CONFIG_UI


#define STYLE(x)			,{\
		.bg_c = CAT(CONFIG_UI_STYLE_BG_C_, INC(INC(x))), \
		.fg_c = CAT(CONFIG_UI_STYLE_FG_C_, INC(INC(x))), \
		.font = & CAT(CONFIG_UI_STYLE_FONT_, INC(INC(x))), \
		.text_color = CAT(CONFIG_UI_STYLE_TEXT_COLOR_, INC(INC(x))), \
		.window_c = CAT(CONFIG_UI_STYLE_WINDOW_C_, INC(INC(x))), \
		.display_c = CAT(CONFIG_UI_STYLE_DISPLAY_C_, INC(INC(x))), \
	}

const uv_uistyle_st uv_uistyles[CONFIG_UI_STYLES_COUNT] = {
		{
				.bg_c = CONFIG_UI_STYLE_BG_C_1,
				.fg_c = CONFIG_UI_STYLE_FG_C_1,
				.font = & CONFIG_UI_STYLE_FONT_1,
				.text_color = CONFIG_UI_STYLE_TEXT_COLOR_1,
				.window_c = CONFIG_UI_STYLE_WINDOW_C_1,
				.display_c = CONFIG_UI_STYLE_DISPLAY_C_1,
		}
		REPEAT(DEC(CONFIG_UI_STYLES_COUNT), STYLE)
};









#endif
