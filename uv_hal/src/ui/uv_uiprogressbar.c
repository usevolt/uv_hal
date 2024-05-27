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
void uv_uiprogressbar_init(void *me, int16_t limit_min,
		int16_t limit_max, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->limit_min = limit_min;
	this->limit_max = limit_max;
	this->font = style->font;
	this->text_c = style->text_color;
	this->main_c = style->fg_c;
	this->bg_c = style->window_c;
	this->horizontal = true;
	this->value_max = this->limit_min;
	this->value_min = this->limit_min;
	this->title = NULL;
	this->cursor_val = this->value_min - 1;
	uv_uiobject_set_draw_callb(this, &draw);
}


void uv_uiprogressbar_set_value(void *me, int16_t value_min, int16_t value_max) {
	LIMITS(value_max, this->limit_min, this->limit_max);
	LIMITS(value_min, this->limit_min, this->limit_max);
	if (this->value_max != value_max) {
		uv_ui_refresh(this);
		this->value_max = value_max;
	}
	if (this->value_min != value_min) {
		uv_ui_refresh(this);
		this->value_min = value_min;
	}
}


void uv_uiprogressbar_set_cursor(void *me, int16_t val) {
	LIMITS(val, this->limit_min, this->limit_max);
	if (this->cursor_val != val) {
		uv_ui_refresh(this);
		this->cursor_val = val;
	}
}



static void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t c = this->main_c;
	int16_t x = this->horizontal ?
			uv_ui_get_xglobal(this) :
			uv_ui_get_xglobal(this) +
			uv_uibb(this)->width / 2 - CONFIG_UI_PROGRESSBAR_HEIGHT / 2;
	int16_t y = this->horizontal ?
			uv_ui_get_yglobal(this) +
			uv_uibb(this)->height / 2 - CONFIG_UI_PROGRESSBAR_HEIGHT / 2 :
			uv_ui_get_yglobal(this) +
			uv_uibb(this)->height -
			(this->title ? (uv_ui_get_string_height(this->title, this->font) + 3) : 0) -
			CONFIG_UI_PROGRESSBAR_WIDTH;
	int16_t rel_max = uv_reli(this->value_max, this->limit_min, this->limit_max),
			rel_min = uv_reli(this->value_min, this->limit_min, this->limit_max);
	LIMITS(rel_max, 0, 1000);
	LIMITS(rel_min, 0, 1000);

	int16_t w, h, barx, bary, barw, barh, cursorpos;
	if (this->horizontal) {
		w = (uv_uibb(this)->width);
		h = CONFIG_UI_PROGRESSBAR_HEIGHT;
		barx = x + w * rel_min / 1000;
		bary = y;
		barw = w * (rel_max - rel_min) / 1000;
		barh = h;
		cursorpos = x +
				w * uv_reli(this->cursor_val, this->limit_min, this->limit_max) / 1000;
	}
	else {
		w = CONFIG_UI_PROGRESSBAR_HEIGHT;
		h = uv_uibb(this)->height;
		barx = x;
		bary = y + h * rel_min / 1000;
		barw = w;
		barh = h * rel_max / 1000;
		cursorpos = y +
				h * uv_reli(this->cursor_val, this->limit_min, this->limit_max) / 1000;
	}

	if (barw < CONFIG_UI_RADIUS + 4) {
		barw = CONFIG_UI_RADIUS + 4;
	}
	else if (barh < CONFIG_UI_RADIUS + 4) {
		barh = CONFIG_UI_RADIUS + 4;
	}

	uv_ui_draw_rrect(x, y, w - 4, h - 4,
			CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, -30));
	uv_ui_draw_rrect(x + 4, y + 4, w - 4, h - 4,
			CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, 30));
	uv_ui_draw_rrect(x + 2, y + 2, w - 4, h - 4,
			CONFIG_UI_RADIUS, this->bg_c);

	// draw bar
	if (rel_min != rel_max) {
		if (this->horizontal) {
			uv_ui_draw_rrect(barx, bary + 2, barw, barh - 4, CONFIG_UI_RADIUS, c);
		}
		else {
			uv_ui_draw_rrect(barx + 2, bary, barw - 4, barh, CONFIG_UI_RADIUS, c);
		}
	}

	// draw cursor
	if (this->cursor_val >= this->value_min &&
			this->cursor_val <= this->value_max) {
		if (this->horizontal) {
			uv_ui_draw_line(cursorpos, y, cursorpos, y + h, 1, this->text_c);
		}
		else {
			uv_ui_draw_line(x, cursorpos, x + w, cursorpos, 1, this->text_c);
		}
	}

	if (this->title) {
		uv_ui_draw_string(this->title, this->font,
				x + w / 2, y + h + 2, ALIGN_TOP_CENTER,
				this->text_c);
	}

}






#endif

