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


#include <ui/uv_uiimage.h>
#include "ui/uv_uilabel.h"

#if CONFIG_UI


static inline void draw(void *me, const uv_bounding_box_st *pbb);


#define this ((uv_uiimage_st*)me)

void uv_uiimage_init(void *me, uv_uimedia_st *media,
		uiimage_wrap_e wrap, alignment_e align) {
	uv_uiobject_init(me);
	this->media = media;
	this->wrap = wrap;
	this->align = align;
	this->blend_c = C(0xFFFFFFFF);

	uv_uiobject_set_draw_callb(this, &draw);
}



static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;

	if (this->media != NULL) {
		int16_t mw = this->media->width;
		int16_t mh = this->media->height;

		if (this->align == ALIGN_CENTER ||
				this->align == ALIGN_TOP_CENTER) {
			x += w / 2 - mw / 2;
		}
		if (this->align == ALIGN_CENTER_RIGHT ||
				this->align == ALIGN_TOP_RIGHT) {
			x += w - mw;
		}
		if (this->align == ALIGN_CENTER ||
				this->align == ALIGN_CENTER_LEFT ||
				this->align == ALIGN_CENTER_RIGHT) {
			y += h / 2 - mh / 2;
		}

		uv_ui_draw_bitmap_ext(this->media, x, y, mw, mh, this->wrap, this->blend_c);
	}
}


void uv_uiimage_set_blendc(void *me, color_t c) {
	if (this->blend_c != c) {
		uv_ui_refresh(this);
	}
	this->blend_c = c;
}


void uv_uiimage_set_media(void *me, uv_uimedia_st *media) {
	if (this->media != media) {
		uv_ui_refresh(this);
		this->media = media;
	}
}



#endif
