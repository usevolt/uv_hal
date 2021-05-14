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


#include "ui/uv_uiprogressbar.h"


#if CONFIG_UI

static void draw(void *me, const uv_bounding_box_st *pbb);

#define this ((uv_uiprogressbar_st *)me)


/// @brief: Initializes the progress bar as horizontal bar
void uv_uiprogressbar_init(void *me, int16_t min_value,
		int16_t max_value, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->min_val = min_value;
	this->max_val = max_value;
	this->font = style->font;
	this->text_c = style->text_color;
	this->main_c = style->fg_c;
	this->bg_c = style->window_c;
	this->horizontal = true;
	this->value = this->min_val;
	this->limit = this->min_val;
	this->limit_type = UI_PROGRESSBAR_LIMIT_NONE;
	this->title = NULL;
	uv_uiobject_set_draw_callb(this, &draw);
}


void uv_uiprogressbar_set_value(void *me, int16_t value) {
	if (value > this->max_val) value = this->max_val;
	else if (value < this->min_val) value = this->min_val;
	if (this->value != value) {
		uv_ui_refresh(this);
		this->value = value;
	}
}


void uv_uiprogressbar_set_limit(void *me, uiprogressbar_limit_e type,
		int16_t limit, color_t color) {
	this->limit_type = type;
	this->limit = limit;
	this->limit_c = color;
	uv_ui_refresh(this);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t c = ((this->limit_type == UI_PROGRESSBAR_LIMIT_UNDER && this->value < this->limit) ||
			(this->limit_type == UI_PROGRESSBAR_LIMIT_OVER && this->value > this->limit)) ?
			this->limit_c :
			this->main_c;
	int16_t x = this->horizontal ?
			uv_ui_get_xglobal(this) :
			uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2 - CONFIG_UI_PROGRESSBAR_HEIGHT / 2;
	int16_t y = this->horizontal ?
			uv_ui_get_yglobal(this) + uv_uibb(this)->height / 2 - CONFIG_UI_PROGRESSBAR_HEIGHT / 2 :
			uv_ui_get_yglobal(this) + uv_uibb(this)->height -
			(this->title ? (uv_ui_get_string_height(this->title, this->font) + 3) : 0) -
			CONFIG_UI_PROGRESSBAR_WIDTH;
	int16_t rel = uv_reli(this->value, this->min_val, this->max_val);
	if (rel < 0) { rel = 0; }

	int16_t w = this->horizontal ?
			(uv_uibb(this)->width) : CONFIG_UI_PROGRESSBAR_HEIGHT;
	int16_t h = this->horizontal ?
			CONFIG_UI_PROGRESSBAR_HEIGHT : uv_uibb(this)->height;
	int16_t barw = (this->horizontal) ?
			w * rel / 1000 : w;
	int16_t barh = (this->horizontal) ?
			h : h * rel / 1000;
	if (barw < CONFIG_UI_RADIUS + 4) {
		barw = CONFIG_UI_RADIUS + 4;
	}
	else if (barh < CONFIG_UI_RADIUS + 4) {
		barh = CONFIG_UI_RADIUS + 4;
	}

	uv_ui_draw_rrect(x, y, w - 4, h - 4, CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, -30));
	uv_ui_draw_rrect(x + 4, y + 4, w - 4, h - 4, CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, 30));
	uv_ui_draw_rrect(x + 2, y + 2, w - 4, h - 4, CONFIG_UI_RADIUS, this->bg_c);

	// draw handle
	uv_ui_draw_rrect(x + 2, y + 2, barw - 4, barh - 4, CONFIG_UI_RADIUS, c);

	if (this->title) {
		uv_ui_draw_string(this->title, this->font,
				x + w / 2, y + h, ALIGN_TOP_CENTER,
				this->text_c);
	}

}






#endif

