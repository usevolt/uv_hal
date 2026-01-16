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



#ifdef this
#undef this
#endif
#define this ((uv_uilistbutton_st*)me)


static void touch(void *me, uv_touch_st *touch);


void uv_uilistbutton_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	char *content = NULL;
	switch(this->content_type) {
		case UILISTBUTTON_CONTENT_ARRAYOFPOINTER:
			content = this->content[this->current_index];
			break;
		default:
			//UILISTBUTTON_CONTENT_ARRAYOFSTRINGS
			// find indexed string
			// start from begin
			content = ((char*) this->content);
			// loop through strings, not calculating zero-length strings
			for (uint16_t i = 0; i < this->current_index; i++) {
				do {
					content += strlen(content) + 1;
				} while (strlen(content) == 0);
			}
			break;
	}

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
	this->content_type = UILISTBUTTON_CONTENT_ARRAYOFPOINTER;
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


#undef this
#define this ((uv_uimedialistbutton_st*)me)



void uv_uimedialistbutton_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	char *content = NULL;
	uv_uilistbutton_st *listthis = (uv_uilistbutton_st*) this;
	switch(listthis->content_type) {
		case UILISTBUTTON_CONTENT_ARRAYOFPOINTER:
			content = listthis->content[listthis->current_index];
			break;
		default:
			//UILISTBUTTON_CONTENT_ARRAYOFSTRINGS
			// find indexed string
			// start from begin
			content = ((char*) listthis->content);
			// loop through strings
			for (uint16_t i = 0; i < listthis->current_index; i++) {
				do {
					content += strlen(content) + 1;
				} while (strlen(content) == 0);
			}
			break;
	}

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
	int16_t bmw = uv_uimedia_get_bitmapwidth(this->bitmap),
			bmh = uv_uimedia_get_bitmapheight(this->bitmap);
		LIMIT_MAX(bmw, w / 2);
		LIMIT_MAX(bmh, h);
		int16_t contentw = bmw + 5 +
			MAX(uv_ui_get_string_width(listthis->title, ((uv_uibutton_st*) this)->font),
				uv_ui_get_string_width(content, ((uv_uibutton_st*) this)->font));
	int16_t textx = x + (w - contentw) / 2 + bmw + (contentw - bmw) / 2 + 5,
			bmx = x + (w - contentw) / 2;
	if (listthis->title) {
		uint16_t th = uv_ui_get_string_height(listthis->title,
				((uv_uibutton_st*) this)->font);
		uint16_t ch = uv_ui_get_string_height(content,
				((uv_uibutton_st*) this)->font);

		uv_ui_draw_string(listthis->title, ((uv_uibutton_st*) this)->font, textx,
				y + ty - (th + ch) / 2 + th / 2, ALIGN_CENTER, fontc);

		ty += (th + ch) / 2 - ch / 2;
	}
	if (uv_uimedia_get_size(this->bitmap) != 0) {
		uv_ui_draw_bitmap_ext(this->bitmap,
							  bmx,
							  y + h / 2 - bmh / 2,
							  bmw,
							  bmh,
							  UIIMAGE_WRAP_BORDER,
							  C(0xFFFFFFFF));
	}
	uv_ui_draw_string(content, ((uv_uibutton_st*) this)->font,
			textx, y + ty, ALIGN_CENTER, fontc);


	int16_t offset = 4;
	if (listthis->content_len) {
		int16_t barw = (w - offset * 2) / (listthis->content_len);

		uv_ui_draw_rrect(x + offset, y + h - offset - CONFIG_UI_LISTBUTTON_BAR_HEIGHT,
				w - offset * 2, CONFIG_UI_LISTBUTTON_BAR_HEIGHT, 0, listthis->bar_c);

		uv_ui_draw_rrect(x + offset +
				(listthis->current_index * (w - offset * 2)) / listthis->content_len,
				y + h - offset - CONFIG_UI_LISTBUTTON_BAR_HEIGHT,
				barw, CONFIG_UI_LISTBUTTON_BAR_HEIGHT, 0, listthis->activebar_c);
	}
}



void uv_uimedialistbutton_init(void *me, char **content,
		uint8_t content_len, uint8_t current_index,
		uv_uimedia_st *bitmap,
		const uv_uistyle_st *style) {
	uv_uilistbutton_init((uv_uilistbutton_st*) this,
						 content,
						 content_len,
						 current_index,
						 style);
	this->bitmap = bitmap;
	uv_uiobject_set_draw_callb(me, &uv_uimedialistbutton_draw);
}


void uv_uimedialistbutton_set_media(void *me, uv_uimedia_st *media) {
	if (this->bitmap != media) {
		uv_ui_refresh(this);
	}
	this->bitmap = media;
}


#endif
