/*
 * uv_ui_styles.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include "ui/uv_ui_styles.h"


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
