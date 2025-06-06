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


#include <ui/uv_uilistbutton.h>
#include "ui/uv_uilabel.h"

#if CONFIG_UI




#define this ((uv_uilistbutton_st*)me)


static void touch(void *me, uv_touch_st *touch);


void uv_uilistbutton_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	char *content = (this->content_string_len == 0) ?
			this->content[this->current_index] :
			(&((char*) this->content)[this->content_string_len * this->current_index]);

	color_t bgc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 20) :
				((uv_uibutton_st*) this)->main_c;
	color_t shadowc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 80) :
				uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -80);
	color_t lightc = (((uv_uibutton_st*) this)->state) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -80) :
				uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 80);
	color_t fontc = ((uv_uibutton_st*) this)->text_c;


	uv_ui_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS,
			 bgc, lightc, shadowc);
	uint16_t ty = h / 2;
	if (this->title) {
		uint16_t th = uv_ui_get_string_height(this->title,
				((uv_uibutton_st*) this)->font);
		uint16_t ch = uv_ui_get_string_height(content,
				((uv_uibutton_st*) this)->font);

		uv_ui_draw_string(this->title, ((uv_uibutton_st*) this)->font, x + w / 2,
				y + ty - (th + ch) / 2 + th / 2, ALIGN_CENTER, fontc);

		ty += (th + ch) / 2 - ch / 2;
	}
	uv_ui_draw_string(content, ((uv_uibutton_st*) this)->font,
			x + w / 2, y + ty, ALIGN_CENTER, fontc);


	int16_t offset = 4;
	if (this->content_len) {
		int16_t barw = (w - offset * 2) / (this->content_len);

		uv_ui_draw_rrect(x + offset, y + h - offset - CONFIG_UI_LISTBUTTON_BAR_HEIGHT,
				w - offset * 2, CONFIG_UI_LISTBUTTON_BAR_HEIGHT, 0, this->bar_c);

		uv_ui_draw_rrect(x + offset +
				(this->current_index * (w - offset * 2)) / this->content_len,
				y + h - offset - CONFIG_UI_LISTBUTTON_BAR_HEIGHT,
				barw, CONFIG_UI_LISTBUTTON_BAR_HEIGHT, 0, this->activebar_c);
	}
}


void uv_uilistbutton_init(void *me, char **content,
		uint8_t content_len, uint8_t current_index, const uv_uistyle_st *style) {
	uv_uibutton_init(this, (char*) content[content_len], style);
	uv_uiobject_set_draw_callb(this, &uv_uilistbutton_draw);
	uv_uiobject_set_touch_callb(this, &touch);
	this->activebar_c = style->fg_c;
	this->bar_c = uv_uic_brighten(style->bg_c, -10);
	this->content_len = content_len;
	this->content = content;
	this->current_index = current_index;
	this->content_string_len = 0;
	if (this->current_index >= this->content_len) {
		this->current_index = 0;
	}
	this->title = NULL;

}



static void touch(void *me, uv_touch_st *touch) {
	_uv_uibutton_touch(this, touch);
	if (uv_uibutton_clicked(this)) {
		this->current_index++;
		if (this->current_index >= this->content_len) {
			this->current_index = 0;
		}
	}
}



#endif
