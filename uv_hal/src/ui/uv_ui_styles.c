/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
