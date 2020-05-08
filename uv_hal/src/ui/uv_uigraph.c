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


#include "uv_uigraph.h"


#if CONFIG_UI



void uv_uigraph_point_init(uv_uigraph_point_st *this,
		int16_t x, int16_t y, bool interactive) {
	this->x = x;
	this->y = y;
	this->interactive = interactive;
}


#define this ((uv_uigraph_st*)me)



void uv_uigraph_init(void *me, uv_uigraph_point_st *points_buffer,
		uint16_t points_count, int16_t min_x, int16_t max_x,
		int16_t min_y, int16_t max_y, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->points = points_buffer;
	this->points_count = points_count;
	this->style = style;
	this->title = NULL;
	this->active_point = -1;
	this->min_x = min_x;
	this->max_x = max_x;
	this->min_y = min_y;
	this->max_y = max_y;
	this->content_w = 0;
	this->content_x = 0;
	this->content_h = 0;
	this->coordinate_c = this->style->text_color;
	this->graph_c = this->style->fg_c;

	uv_uiobject_set_draw_callb(this, &uv_uigraph_draw);
	uv_uiobject_set_touch_callb(this, &uv_uigraph_touch);
	uv_uiobject_set_step_callb(this, &uv_uigraph_step);
}



void uv_uigraph_draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = uv_ui_get_yglobal(this);
	int16_t w = uv_uibb(this)->width;
	int16_t h = uv_uibb(this)->height;
	int16_t title_h = (this->title == NULL) ? 0 :
			uv_ft81x_get_string_height(this->title, this->style->font);
	int16_t ch = h - title_h;
	int16_t cw = w;
	int16_t value_height = CONFIG_UI_GRAPH_LINE_WIDTH + this->style->font->char_height;
	char str[20];
	sprintf(str, "%i", this->max_y);
	int16_t maxy_value_width = uv_ft81x_get_string_width(str, this->style->font);
	sprintf(str, "%i", this->min_y);
	int16_t miny_value_width = uv_ft81x_get_string_width(str, this->style->font);
	int16_t value_width = MAX(maxy_value_width, miny_value_width);

	// draw the coordinate lines and text values on edges
	// x axis
	int32_t rel = uv_reli(0, this->min_y, this->max_y);
	rel = uv_lerpi(rel, y + ch, y);
	// check if the x-axis is drawn too low, so that the values won't fit under it.
	// In this case make the content a little more narrow.
	if (rel > y + ch - value_height) {
		ch -= rel - (y + ch - value_height);
	}
	LIMITS(rel, y, y + ch);
	uv_ft81x_draw_line(x, rel, x + w, rel, CONFIG_UI_GRAPH_LINE_WIDTH, this->coordinate_c);
	sprintf(str, "%i", this->min_x);
	uv_ft81x_draw_string(str, this->style->font, x, rel + CONFIG_UI_GRAPH_LINE_WIDTH,
			ALIGN_TOP_LEFT, this->coordinate_c);
	sprintf(str, "%i", this->max_x);
	uv_ft81x_draw_string(str, this->style->font, x + w, rel + CONFIG_UI_GRAPH_LINE_WIDTH,
			ALIGN_TOP_RIGHT, this->coordinate_c);

	// y axis
	rel = uv_reli(0, this->min_x, this->max_x);
	rel = uv_lerpi(rel, x, x + cw);
	if (rel < x + value_width) {
		int16_t v = rel - (x + cw - value_width);
		x += v;
		// update the content_x variable
		this->content_x += v;
		cw -= v;
	}
	LIMITS(rel, x, x + cw);
	uv_ft81x_draw_line(rel, y, rel, y + ch, CONFIG_UI_GRAPH_LINE_WIDTH, this->coordinate_c);
	sprintf(str, "%i", this->max_y);
	uv_ft81x_draw_string(str, this->style->font, rel, y,
			ALIGN_TOP_RIGHT, this->coordinate_c);
	sprintf(str, "%i", this->min_y);
	uv_ft81x_draw_string(str, this->style->font, rel,
			y + ch - uv_ft81x_get_string_height(str, this->style->font),
			ALIGN_TOP_RIGHT, this->coordinate_c);

	// draw the lines between the points
	int16_t last_px = 0;
	int16_t last_py = 0;
	int16_t inter_px = 0;
	int16_t inter_py = 0;
	bool interactive = false;
	for (uint16_t i = 0; i < this->points_count; i++) {
		uv_uigraph_point_st *p = &this->points[i];
		int16_t px = uv_lerpi(uv_reli(p->x, this->min_x, this->max_x), x, x + cw);
		int16_t py = uv_lerpi(uv_reli(p->y, this->min_y, this->max_y), y + ch, y);
		// draw the line connecting the points
		if (i != 0) {
			uv_ft81x_draw_line(last_px, last_py, px, py,
					CONFIG_UI_GRAPH_LINE_WIDTH, this->graph_c);
		}
		if (p->interactive) {
			inter_px = px;
			inter_py = py;
			interactive = true;
		}
		last_px = px;
		last_py = py;
	}
	// after the lines draw the interactive point's dot
	if (interactive) {
		uv_ft81x_draw_shadowpoint(inter_px, inter_py,
				this->style->fg_c,
				uv_uic_brighten(this->style->fg_c, 30),
				uv_uic_brighten(this->style->fg_c, -30),
				this->style->font->char_height);
	}

	// draw the title text if assigned
	if (this->title != NULL) {
		uv_ft81x_draw_string(this->title, this->style->font,
				x + w / 2, y + h - title_h, ALIGN_TOP_CENTER, this->style->text_color);
	}

	this->content_w = cw;
	this->content_h = ch;

}



void uv_uigraph_touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		bool found = false;
		for (uint16_t i = 0; i < this->points_count; i++) {
			uv_uigraph_point_st *p = &this->points[i];
			int16_t px = uv_lerpi(uv_reli(p->x, this->min_x, this->max_x),
					this->content_x, this->content_x + this->content_w);
			int16_t py = uv_lerpi(uv_reli(p->y, this->min_y, this->max_y),
					this->content_h, 0);

			if (touch->x > px - this->style->font->char_height &&
					touch->x < px + this->style->font->char_height &&
					touch->y > py - this->style->font->char_height &&
					touch->y < py + this->style->font->char_height) {
				this->active_point = i;
				// prevent the touch to propagate any further
				touch->action = TOUCH_NONE;
				found = true;
				break;
			}
		}
		if (found == false) {
			// no point clicked
			this->active_point = -1;
			touch->action = TOUCH_NONE;
		}
		uv_ui_refresh(this);
	}

}



uv_uiobject_ret_e uv_uigraph_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;



	return ret;
}




#endif
