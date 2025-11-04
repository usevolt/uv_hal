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


#include <stdlib.h>
#include <ui/uv_uislider.h>

#if CONFIG_UI

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);
static uv_uiobject_ret_e step(void *me, uint16_t step_ms);


#define this ((uv_uislider_st*) me)


void uv_uislider_init(void *me, int16_t min_value, int16_t max_value, int16_t current_value,
		const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->min_val = min_value;
	this->max_val = max_value;
	this->cur_val = current_value;
	if (this->cur_val > this->max_val) this->cur_val = this->max_val;
	else if (this->cur_val < this->min_val) this->cur_val = this->min_val;
	this->font = style->font;
	this->text_c = style->text_color;
	this->handle_c = style->fg_c;
	this->bg_c = style->bg_c;
	this->horizontal = true;
	this->show_value = true;
	this->dragging = false;
	this->inc_step = 1;
	this->drag_val = 0;
	this->title = NULL;
	uv_delay_init(&this->longpress_delay, UISLIDER_LONGPRESS_DELAY_MS);
	uv_delay_end(&this->longpress_delay);
	this->incdec = 0;
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
	uv_uiobject_set_step_callb(this, &step);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x, y, w, h;
	if (this->horizontal) {
		x = uv_ui_get_xglobal(this);
		y = uv_ui_get_yglobal(this) + CONFIG_UI_SLIDER_WIDTH / 2 - CONFIG_UI_SLIDER_WIDTH / 2 -
				((this->title) ? uv_ui_get_font_height(this->font) / 2 : 0);
		if (uv_uibb(this)->height > CONFIG_UI_SLIDER_WIDTH) {
			y += (uv_uibb(this)->height - CONFIG_UI_SLIDER_WIDTH) / 2;
		}
		w = uv_uibb(this)->width;
		h = CONFIG_UI_SLIDER_WIDTH;
		// handle relative position
		int16_t hpx = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hx = uv_lerpi(hpx, 0, uv_uibb(this)->width - CONFIG_UI_SLIDER_WIDTH - 1);

		uv_ui_draw_shadowrrect(x, y + h / 2 - 5, w, 10, CONFIG_UI_RADIUS,
				this->bg_c, uv_uic_brighten(this->bg_c, 80), uv_uic_brighten(this->bg_c, -80));
		// handle
		uv_ui_draw_shadowrrect(x + hx, y, CONFIG_UI_SLIDER_WIDTH, h, CONFIG_UI_RADIUS,
				this->handle_c, uv_uic_brighten(this->handle_c, 80),
				uv_uic_brighten(this->handle_c, -80));
		// handle text
		if (this->show_value) {
			char str[10] = {};
			snprintf(str, sizeof(str) - 1, "%i", this->cur_val);
			uv_ui_draw_string(str, this->font,
					x + hx + CONFIG_UI_SLIDER_WIDTH / 2, y + (h / 2),
					ALIGN_CENTER, this->text_c);
		}
	}
	else {
		x = uv_ui_get_xglobal(this) + CONFIG_UI_SLIDER_WIDTH / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		if (uv_uibb(this)->width > CONFIG_UI_SLIDER_WIDTH) {
			x += (uv_uibb(this)->w - CONFIG_UI_SLIDER_WIDTH) / 2;
		}
		y = uv_ui_get_yglobal(this);
		w = CONFIG_UI_SLIDER_WIDTH;
		h = uv_uibb(this)->height - (this->title ?
				(uv_ui_get_string_height(this->title, this->font) + 5) : 0);
		// draw slider handle
		// handle relative position
		int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height -
				CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ?
						(uv_ui_get_string_height(this->title, this->font) + 5) : 0), 0);

		uv_ui_draw_rrect(x + w / 2 - 7, y, 10, h - 5,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, -80));
		uv_ui_draw_rrect(x + 4 + w / 2 - 7, y + 4, 10, h - 5,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, 80));
		uv_ui_draw_rrect(x + 2 + w / 2 - 7, y + 2, 10, h - 5,
				CONFIG_UI_RADIUS, this->bg_c);
		// handle
		uv_ui_draw_rrect(x, y + hy, w - 4, CONFIG_UI_SLIDER_WIDTH - 4,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->handle_c, -80));
		uv_ui_draw_rrect(x + 4, y + hy + 4, w - 4, CONFIG_UI_SLIDER_WIDTH - 4,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->handle_c, 80));
		uv_ui_draw_rrect(x + 2, y + hy + 2, w - 4, CONFIG_UI_SLIDER_WIDTH - 4,
				CONFIG_UI_RADIUS, this->handle_c);
		// handle text
		if (this->show_value) {
			char str[10] = {};
			snprintf(str, sizeof(str) - 1, "%i", this->cur_val);
			uv_ui_draw_string(str, this->font,
					x + (w / 2), y + hy + CONFIG_UI_SLIDER_WIDTH / 2,
					ALIGN_CENTER, this->text_c);
		}
	}

	uv_ui_draw_string(this->title, this->font, x + w / 2, y + h + 5,
			ALIGN_TOP_CENTER, this->text_c);


}



static void touch(void *me, uv_touch_st *touch) {
	if (touch->action == TOUCH_PRESSED) {
		this->dragging = true;
		// prevent action from propagating to other elements
		touch->action = TOUCH_NONE;
		this->drag_start_val = this->cur_val;
	}
	else if (touch->action == TOUCH_DRAG && this->dragging) {
		this->drag_val += (this->horizontal) ? touch->x : -touch->y;
		this->cur_val = this->drag_start_val +
				this->drag_val * (this->max_val - this->min_val) /
				(((this->horizontal) ? uv_uibb(this)->width : uv_uibb(this)->height)
						- CONFIG_UI_SLIDER_WIDTH);
		if (this->cur_val < this->min_val) { this->cur_val = this->min_val; }
		else if (this->cur_val > this->max_val) { this->cur_val = this->max_val; }
		else { }

		// prevent action from propagating into other elements
		touch->action = TOUCH_NONE;
		uv_ui_refresh(this);
	}
	else if (touch->action == TOUCH_IS_DOWN) {
		this->drag_start_val = this->cur_val;
		if (uv_delay_has_ended(&this->longpress_delay)) {
			uv_delay_init(&this->longpress_delay,
					(this->incdec == 0) ?
							UISLIDER_LONGPRESS_DELAY_MS :
							UISLIDER_LONGPRESS_MIN_DELAY_MS);

			// if incdec was 0, it means that the uislider was pressed for the first step cycle
			// and we should determine if we want to increase or decrease the value
			if (this->incdec == 0) {
				this->incdec = this->inc_step;

				if (this->horizontal) {
					int16_t hpx = uv_reli(this->cur_val, this->min_val, this->max_val);
					int16_t hx = uv_lerpi(hpx, 0,
							uv_uibb(this)->width - CONFIG_UI_SLIDER_WIDTH - 1);
					if (touch->x < hx + CONFIG_UI_SLIDER_WIDTH / 2) {
						this->incdec = -this->inc_step;
					}
				}
				else {
					int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
					int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height -
							CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ? this->font->char_height + 5 : 0), 0);
					if (touch->y > hy + CONFIG_UI_SLIDER_WIDTH / 2) {
						this->incdec = -this->inc_step;
					}
				}
			}
			// round up to the closest inc_step value
			int8_t i = this->incdec;
			if ((this->cur_val % this->inc_step) != 0) {
				if (i < 0) {
					i = -(this->cur_val % this->inc_step);
				}
				else {
					i = this->inc_step - (this->cur_val % this->inc_step);
				}
			}
			uv_uislider_set_value(this, this->cur_val + i);
		}
		// prevent action from propagating into other elements
		touch->action = TOUCH_NONE;
	}
	else if (touch->action == TOUCH_NONE && this->dragging) {
		this->dragging = false;
		this->drag_val = 0;
	}
	else if (touch->action == TOUCH_CLICKED) {
		// prevent action from propagating into other elements
		touch->action = TOUCH_NONE;
	}
	else {
		uv_delay_end(&this->longpress_delay);
		this->incdec = 0;
	}

}


static uv_uiobject_ret_e step(void *me, uint16_t step_ms) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	uv_delay(&this->longpress_delay, step_ms);

	return ret;
}



/// @brief: Sets the current value
void uv_uislider_set_value(void *me, int16_t value) {
	if (value < this->min_val) value = this->min_val;
	else if (value > this->max_val) value = this->max_val;
	if (value != this->cur_val) {
		uv_ui_refresh(this);
	}
	this->cur_val = value;
}


/// @brief: Sets the minimum value
void uv_uislider_set_min_value(void *me, int16_t min_value) {
	if (this->min_val != min_value) {
		this->min_val = min_value;
		uv_uislider_set_value(this, this->cur_val);
		uv_ui_refresh(this);
	}
}

/// @brief: sets the maximum value
void uv_uislider_set_max_value(void *me, int16_t max_value) {
	if (this->max_val != max_value) {
		this->max_val = max_value;
		uv_uislider_set_value(this, this->cur_val);
		uv_ui_refresh(this);
	}
}




#endif
