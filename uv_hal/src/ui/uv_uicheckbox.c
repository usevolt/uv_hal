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



#include "ui/uv_uicheckbox.h"


#if CONFIG_UI

#define this ((uv_uicheckbox_st*)me)



static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t fontc = ((uv_uibutton_st *) this)->text_c;
	color_t shadowc = uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30);
	color_t lightc = uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30);
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t bh = uv_ft81x_get_string_height(((uv_uibutton_st*) this)->text,
			((uv_uibutton_st*) this)->font) +
					uv_ft81x_get_font_height(((uv_uibutton_st*) this)->font);

	uv_ft81x_draw_shadowrrect(x, y + uv_uibb(this)->height / 2 - bh / 2, bh, bh, CONFIG_UI_RADIUS,
			((uv_uibutton_st*) this)->main_c, lightc, shadowc);
	if (uv_uitogglebutton_get_state((uv_uitogglebutton_st*) this)) {
		const uint8_t offset = 6;
		uv_ft81x_draw_rrect(x + offset, y + uv_uibb(this)->height / 2 - bh / 2 + offset,
				bh - offset * 2, bh - offset * 2, CONFIG_UI_RADIUS, this->fillc);
	}
	uv_ft81x_draw_string(((uv_uibutton_st*) this)->text, ((uv_uibutton_st*) this)->font, x + bh * 3 / 2,
			y + uv_uibb(this)->height / 2, ALIGN_CENTER_LEFT, fontc);
}

void uv_uicheckbox_init(void *me, bool state, char *text, const uv_uistyle_st *style) {
	uv_uitogglebutton_init(me, state, text, style);
	uv_uiobject_set_draw_callb(this, &draw);
	this->fillc = style->fg_c;
}





#endif
