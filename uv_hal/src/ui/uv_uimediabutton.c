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



#include "ui/uv_uimediabutton.h"



#if CONFIG_UI

#define this ((uv_uimediabutton_st*)me)

static void draw_common(void *me, bool pressed, const uv_bounding_box_st *pbb);


static void draw_mediabutton(void *me, const uv_bounding_box_st *pbb) {
	draw_common(me, uv_uibutton_is_down(me), pbb);
}

static void draw_mediatogglebutton(void *me, const uv_bounding_box_st *pbb) {
	draw_common(me, uv_uitogglebutton_get_state(me), pbb);
}



static void draw_common(void *me, bool pressed, const uv_bounding_box_st *pbb) {
	color_t fontc = ((uv_uibutton_st *) this)->text_c;
	color_t bgc = (pressed) ?
			((uv_uitogglebutton_st*) this)->active_c :
			((uv_uibutton_st*) this)->main_c;
	color_t shadowc = (pressed) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30);
	color_t lightc = (pressed) ?
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, -30) :
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30);
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	uint8_t offset = 10;
	uv_ui_draw_shadowrrect(x, y, w, h, CONFIG_UI_RADIUS, bgc, lightc, shadowc);
	if (this->align == UIMEDIABUTTON_ALIGN_HORIZONTAL) {
		int16_t imgw = 0;
		if (this->media != NULL) {
			imgw = uv_uimedia_get_bitmapwidth(this->media);
			uv_ui_draw_bitmap(this->media, x + ((strlen(((uv_uibutton_st*) this)->text) == 0) ?
						((w - uv_uimedia_get_bitmapwidth(this->media)) / 2) : offset),
					y + h / 2 - uv_uimedia_get_bitmapheight(this->media) / 2);
		}
		uv_ui_draw_string(((uv_uibutton_st*) this)->text,
				((uv_uibutton_st*) this)->font, x + imgw + (w - imgw) / 2,
				y + h / 2, ALIGN_CENTER, fontc);
	}
	else {
		int16_t contenth = uv_uimedia_get_bitmapheight(this->media) +
				uv_ui_get_string_height(((uv_uibutton_st*) this)->text,
						((uv_uibutton_st*)this)->font);
		int16_t space = (uv_uibb(me)->height - contenth) /
				((*((uv_uibutton_st*) this)->text == '\0') ? 2 : 3);
		if (this->media != NULL) {
			uv_ui_draw_bitmap(this->media,
					x + w / 2 - uv_uimedia_get_bitmapwidth(this->media) / 2,
					MAX(y + space, y));
		}
		uv_ui_draw_string(((uv_uibutton_st*) this)->text,
				((uv_uibutton_st*) this)->font, x + w / 2,
				MIN((y + space * 2 + uv_uimedia_get_bitmapheight(this->media)),
						y + h - uv_ui_get_string_height(
								((uv_uibutton_st*) this)->text,
								((uv_uibutton_st*) this)->font)),
						ALIGN_TOP_CENTER, fontc);

	}
}

void uv_uimediabutton_init(void *me, char *text,
		uv_uimedia_st *media, const uv_uistyle_st *style) {
	uv_uibutton_init(me, text, style);
	uv_uitogglebutton_set_active_c(me,
			uv_uic_brighten(((uv_uibutton_st*) this)->main_c, 30));
	if (media != NULL &&
			media->size == 0) {
		this->media = NULL;
	}
	else {
		this->media = media;
	}
	this->align = UIMEDIABUTTON_ALIGN_HORIZONTAL;
	uv_uiobject_set_step_callb(this, &uv_uibutton_step);
	uv_uiobject_set_draw_callb(this, &draw_mediabutton);
}




void uv_uimediabutton_set_media(void *me, uv_uimedia_st *media) {
	if (this->media != media) {
		this->media = media;
		uv_ui_refresh(this);
	}
}













void uv_uimediatogglebutton_init(void *me, bool state, char *text,
		uv_uimedia_st *media, const uv_uistyle_st *style) {
	uv_uimediabutton_init(me, text, media, style);
	uv_uitogglebutton_init(me, state, text, style);
	uv_uiobject_set_draw_callb(this, &draw_mediatogglebutton);
	uv_uiobject_set_step_callb(this, &uv_uitogglebutton_step);
}






#endif
