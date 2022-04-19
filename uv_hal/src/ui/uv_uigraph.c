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
	this->clicked = false;
	this->active_point = -1;
	this->min_x = min_x;
	this->max_x = max_x;
	this->min_y = min_y;
	this->max_y = max_y;
	this->content_w = 0;
	this->content_x = 0;
	this->content_h = 0;
	this->current_val_x = this->min_x - 1;
	this->current_val_y = this->min_y - 1;
	this->coordinate_c = this->style->text_color;
	this->graph_c = this->style->fg_c;
	this->x_unit = NULL;
	this->y_unit = NULL;
	this->point_moved_callb = NULL;

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
			uv_ui_get_string_height(this->title, this->style->font);
	int16_t ch = h - title_h;
	int16_t cw = w;
	int16_t value_height = CONFIG_UI_GRAPH_LINE_WIDTH + this->style->font->char_height;
	char str[20];
	if (this->y_unit != NULL) {
		sprintf(str, "%i %s", this->max_y, this->y_unit);
	}
	else {
		sprintf(str, "%i", this->max_y);
	}
	int16_t maxy_value_width = uv_ui_get_string_width(str, this->style->font);
	if (this->y_unit != NULL) {
		sprintf(str, "%i %s", this->min_y, this->y_unit);
	}
	else {
		sprintf(str, "%i", this->min_y);
	}
	int16_t miny_value_width = uv_ui_get_string_width(str, this->style->font);
	int16_t value_width = MAX(maxy_value_width, miny_value_width) + CONFIG_UI_GRAPH_LINE_WIDTH;

	this->content_x = 0;
	this->content_w = w;
	this->content_h = h;

	// draw the coordinate lines and text values on edges
	// x axis
	int32_t rely = uv_reli(0, this->min_y, this->max_y);
	rely = uv_lerpi(rely, y + ch, y);
	// check if the x-axis is drawn too low, so that the values won't fit under it.
	// In this case make the content a little more narrow.
	if (rely > y + ch - value_height) {
		ch -= rely - (y + ch - value_height);
	}
	LIMITS(rely, y, y + ch);

	// y axis
	int32_t relx = uv_reli(0, this->min_x, this->max_x);
	relx = uv_lerpi(relx, x, x + cw);
	if (relx < x + value_width) {
		int16_t v = relx - (x - value_width);
		x += v;
		// update the content_x variable
		this->content_x += v;
		cw -= v;
	}
	LIMITS(relx, x, x + cw);

	// draw the current value lines
	int16_t cx = uv_lerpi(uv_reli(this->current_val_x, this->min_x, this->max_x), x, x + cw);
	int16_t cy = uv_lerpi(uv_reli(this->current_val_y, this->min_y, this->max_y), y + ch, y);
	if (this->current_val_x >= this->min_x &&
			this->current_val_x <= this->max_x) {
		uv_ui_draw_line(cx, y, cx, y + ch, CONFIG_UI_GRAPH_LINE_WIDTH, this->style->bg_c);
	}
	if (this->current_val_y >= this->min_y &&
			this->current_val_y <= this->max_y) {
		uv_ui_draw_line(x, cy, x + cw, cy, CONFIG_UI_GRAPH_LINE_WIDTH, this->style->bg_c);
	}

	// draw x axis
	uv_ui_draw_line(x, rely, x + cw, rely, CONFIG_UI_GRAPH_LINE_WIDTH, this->coordinate_c);
	if (this->x_unit != NULL) {
		sprintf(str, "%i %s", this->min_x, this->x_unit);
	}
	else {
		sprintf(str, "%i", this->min_x);
	}
	uv_ui_draw_string(str, this->style->font, x, rely + CONFIG_UI_GRAPH_LINE_WIDTH,
			ALIGN_TOP_LEFT, this->coordinate_c);
	if (this->x_unit != NULL) {
		sprintf(str, "%i %s", this->max_x, this->x_unit);
	}
	else {
		sprintf(str, "%i", this->max_x);
	}
	uv_ui_draw_string(str, this->style->font, x + cw, rely + CONFIG_UI_GRAPH_LINE_WIDTH,
			ALIGN_TOP_RIGHT, this->coordinate_c);

	// draw y axis
	uv_ui_draw_line(relx, y, relx, y + ch, CONFIG_UI_GRAPH_LINE_WIDTH, this->coordinate_c);
	if (this->y_unit != NULL) {
		sprintf(str, "%i %s", this->max_y, this->y_unit);
	}
	else {
		sprintf(str, "%i", this->max_y);
	}
	uv_ui_draw_string(str, this->style->font, relx - CONFIG_UI_GRAPH_LINE_WIDTH, y,
			ALIGN_TOP_RIGHT, this->coordinate_c);
	if (this->y_unit != NULL) {
		sprintf(str, "%i %s", this->min_y, this->y_unit);
	}
	else {
		sprintf(str, "%i", this->min_y);
	}
	uv_ui_draw_string(str, this->style->font, relx - CONFIG_UI_GRAPH_LINE_WIDTH,
			y + ch - uv_ui_get_string_height(str, this->style->font),
			ALIGN_TOP_RIGHT, this->coordinate_c);


	// draw the title text if assigned
	if (this->title != NULL) {
		uv_ui_draw_string(this->title, this->style->font,
				x + w / 2, y + h - title_h, ALIGN_TOP_CENTER, this->style->text_color);
	}


	// draw the lines between the points
	int16_t last_px = 0;
	int16_t last_py = 0;
	uv_ui_set_mask(x, y, cw, ch);
	for (uint16_t i = 0; i < this->points_count; i++) {
		uv_uigraph_point_st *p = &this->points[i];
		int16_t px = uv_lerpi(uv_reli(p->x, this->min_x, this->max_x), x, x + cw);
		int16_t py = uv_lerpi(uv_reli(p->y, this->min_y, this->max_y), y + ch, y);
		// draw the line connecting the points
		if (i != 0) {
			uv_ui_draw_line(last_px, last_py, px, py,
					CONFIG_UI_GRAPH_LINE_WIDTH, this->graph_c);
		}
		last_px = px;
		last_py = py;
	}
	// after the lines draw the point dots
	for (uint16_t i = 0; i < this->points_count; i++) {
		uv_uigraph_point_st *p = &this->points[i];
		if (p->interactive) {
			int16_t px = uv_lerpi(uv_reli(p->x, this->min_x, this->max_x), x, x + cw);
			int16_t py = uv_lerpi(uv_reli(p->y, this->min_y, this->max_y), y + ch, y);
			color_t c = this->graph_c;
			if (i == this->active_point) {
				uv_ui_draw_shadowpoint(px, py, c,
						uv_uic_brighten(c, 30), uv_uic_brighten(c, -30),
						this->style->font->char_height * 3 / 2);
			}
			else {
				uv_ui_draw_point(px, py, c, this->style->font->char_height);
			}
		}
	}

	// draw the active point's coordinates
	if (this->active_point != -1) {
		uv_uigraph_point_st *p = &this->points[this->active_point];
		char str[40];
		if (this->x_unit != NULL) {
			sprintf(str, "(%i %s, ", p->x, this->x_unit);
		}
		else {
			sprintf(str, "(%i, ", p->x);
		}
		if (this->y_unit != NULL) {
			sprintf(str + strlen(str), "%i %s)", p->y, this->y_unit);
		}
		else {
			sprintf(str + strlen(str), "%i)", p->y);
		}
		int16_t w = uv_ui_get_string_width(str, this->style->font),
				h = uv_ui_get_string_height(str, this->style->font);
		int16_t px = uv_lerpi(uv_reli(p->x, this->min_x, this->max_x), x, x + cw);
		int16_t py = uv_lerpi(uv_reli(p->y, this->min_y, this->max_y), y + ch, y);
		int16_t offset = this->style->font->char_height;
		int16_t tx = px + offset,
				ty = py - offset - h;
		if (p->x > this->max_x - (this->max_x - this->min_x) / 2) {
			// draw the text on the left side of the point
			tx = px - offset - w;
		}
		if (p->y > this->max_y - (this->max_y - this->min_y) / 2) {
			ty = py + offset;
		}
		printf("drawing %s on (%i %i)\n", str, tx, ty);
		uv_ui_draw_string(str, this->style->font, tx, ty,
				ALIGN_TOP_LEFT, this->style->text_color);
	}


	this->content_w = cw;
	this->content_h = ch;

}



void uv_uigraph_touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_CLICKED) {
		for (uint16_t i = MAX(0, this->active_point); i < this->points_count; i++) {
			if (this->points[i].interactive) {
				uv_uigraph_point_st *p = &this->points[i];
				int16_t px = uv_lerpi(uv_reli(p->x, this->min_x, this->max_x),
						this->content_x, this->content_x + this->content_w);
				int16_t py = uv_lerpi(uv_reli(p->y, this->min_y, this->max_y),
						this->content_h, 0);

				if (this->active_point == -1 ||
						this->active_point == i) {
					if (touch->x > px - this->style->font->char_height * 2 &&
							touch->x < px + this->style->font->char_height * 2 &&
							touch->y > py - this->style->font->char_height * 2 &&
							touch->y < py + this->style->font->char_height * 2) {
						if (this->active_point == -1) {
							// new point selected
							this->active_point = i;
							// prevent the touch to propagate any further
							touch->action = TOUCH_NONE;
							break;
						}
						else {
							// deselect this point
							this->active_point = -1;
							// but continue looping through other points as they
							// can be selected
						}
					}
				}
			}
		}
		this->clicked = true;
		uv_ui_refresh(this);
	}

}



uv_uiobject_ret_e uv_uigraph_step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	this->clicked = false;

	return ret;
}




void uv_uigraph_set_current_val(void *me, int16_t val_x, int16_t val_y) {
	if (this->current_val_x != val_x ||
			this->current_val_y != val_y) {
		uv_ui_refresh(this);
	}
	this->current_val_x = val_x;
	this->current_val_y = val_y;
}




#endif
