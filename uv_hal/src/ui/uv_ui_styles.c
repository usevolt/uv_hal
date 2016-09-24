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

const uv_window_style_st uv_window_styles[CONFIG_UI_WINDOW_STYLES_COUNT] = {
		{
				.main_color= CONFIG_UI_WINDOW_COLOR_1,
				.frame_color = CONFIG_UI_WINDOW_FRAME_COLOR_1,
				.frame_thickness = CONFIG_UI_WINDOW_FRAME_THICKNESS_1
		}
		REPEAT(DEC(CONFIG_UI_WINDOW_STYLES_COUNT), WINDOW_STYLE)

};

