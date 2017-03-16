/*
 * uslider.c
 *
 *  Created on: Aug 19, 2016
 *      Author: usevolt
 */


#include <stdlib.h>
#include <ui/uv_uislider.h>

#if CONFIG_LCD

#define this ((uv_uislider_st*) me)


void uv_uislider_init(void *me, int16_t min_value, int16_t max_value, int16_t current_value,
		const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->min_val = min_value;
	this->max_val = max_value;
	this->cur_val = current_value;
	if (this->cur_val > this->max_val) this->cur_val = this->max_val;
	else if (this->cur_val < this->min_val) this->cur_val = this->min_val;
	this->style = style;
	this->horizontal = true;
	this->show_value = true;
	this->dragging = false;
	this->inc_step = 1;
	this->drag_val = 0;
	this->title = NULL;
}


static void draw(void *me) {
	int16_t x, y, w, h;
	if (this->horizontal) {
		if (uv_uibb(this)->height < CONFIG_UI_SLIDER_WIDTH) {
			this->super.bb.height = CONFIG_UI_SLIDER_WIDTH;
		}
		x = uv_ui_get_xglobal(this);
		y = uv_ui_get_yglobal(this) + uv_uibb(this)->height / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		w = uv_uibb(this)->width;
		h = CONFIG_UI_SLIDER_WIDTH;
		uv_lcd_draw_rect(x, y, w, h, this->style->inactive_bg_c);
		uv_lcd_draw_frame(x, y, w, h, 1,
				this->style->inactive_frame_c);
		// draw slider handle
		// handle relative position
		int16_t hpx = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hx = uv_lerpi(hpx, 0, uv_uibb(this)->width - CONFIG_UI_SLIDER_WIDTH - 1);
		// hx indicates the handle position
		uv_lcd_draw_rect(x + hx + 1, y + 1,
				CONFIG_UI_SLIDER_WIDTH - 1, h - 2,
				this->style->active_fg_c);
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			_uv_ui_draw_text(x + w/2, y + h/2, this->style->font, ALIGN_CENTER,
					this->style->inactive_font_c, C(0xFFFFFFFF), str, 1.0f);
		}
		_uv_ui_draw_text(x + 1, y + h / 2, this->style->font, ALIGN_CENTER_LEFT,
				this->style->text_color, C(0xFFFFFFFF), "\x11", 1.0f);
		_uv_ui_draw_text(x + w - 1, y + h / 2, this->style->font, ALIGN_CENTER_RIGHT,
				this->style->text_color, C(0xFFFFFFFF), "\x10", 1.0f);
	}
	else {
		if (uv_uibb(this)->width < CONFIG_UI_SLIDER_WIDTH) {
			this->super.bb.width = CONFIG_UI_SLIDER_WIDTH;
		}
		x = uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		y = uv_ui_get_yglobal(this);
		w = CONFIG_UI_SLIDER_WIDTH;
		h = uv_uibb(this)->height - (this->title ?
				(uv_ui_text_height_px(this->title, this->style->font, 1.0f) + 5) : 0);
		uv_lcd_draw_rect(x, y, w, h, this->style->inactive_bg_c);
		uv_lcd_draw_frame(x, y, w, h, 1,
				this->style->inactive_frame_c);
		// draw slider handle
		// handle relative position
		int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height -
				CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ?
						(uv_ui_text_height_px(this->title, this->style->font, 1.0f) + 5) : 0), 0);
		// hy indicates the handle position
		uv_lcd_draw_rect(x + 1, y + hy + 1,
				w - 2, CONFIG_UI_SLIDER_WIDTH - 1,
				this->style->active_fg_c);
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			_uv_ui_draw_text(x + w/2, y + h/2, this->style->font, ALIGN_CENTER,
					this->style->inactive_font_c, C(0xFFFFFFFF), str, 1.0f);
		}
		_uv_ui_draw_text(x + w / 2, y + 1, this->style->font, ALIGN_TOP_CENTER,
				this->style->text_color, C(0xFFFFFFFF), "\x1E", 1.0f);
		_uv_ui_draw_text(x + w / 2, y + h - 1, this->style->font, ALIGN_BOTTOM_CENTER,
				this->style->text_color, C(0xFFFFFFFF), "\x1F", 1.0f);
	}
	_uv_ui_draw_text(x + w/2, y + h + 5, this->style->font, ALIGN_TOP_CENTER,
			this->style->text_color, C(0xFFFFFFFF), this->title, 1.0f);


}


void uv_uislider_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
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
		if (this->cur_val < this->min_val) this->cur_val = this->min_val;
		else if (this->cur_val > this->max_val) this->cur_val = this->max_val;

		// prevent action from propagating into other elements
		touch->action = TOUCH_NONE;
		uv_ui_refresh(this);
	}
	else if (touch->action == TOUCH_CLICKED) {
		int8_t i = this->inc_step;

		if (this->horizontal) {
			int16_t hpx = uv_reli(this->cur_val, this->min_val, this->max_val);
			int16_t hx = uv_lerpi(hpx, 0,
					uv_uibb(this)->width - CONFIG_UI_SLIDER_WIDTH - 1);
			if (touch->x < hx + CONFIG_UI_SLIDER_WIDTH / 2) {
				i = -this->inc_step;
			}
		}
		else {
			int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
			int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height -
					CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ? this->style->font->char_height + 5 : 0), 0);
			if (touch->y > hy + CONFIG_UI_SLIDER_WIDTH / 2) {
				i = -this->inc_step;
			}
		}
//		not tested
//		if (this->cur_val % i) {
//			i = this->cur_val % i;
//		}
		uv_uislider_set_value(this, this->cur_val + i);
	}
	else if (touch->action == TOUCH_NONE && this->dragging) {
		this->dragging = false;
		this->drag_val = 0;
		// prevent action from propagating to other elements
		touch->action = TOUCH_NONE;
	}


	if (this->super.refresh && this->super.visible) {
		draw(this);
	}
}


/// @brief: Sets the current value
void uv_uislider_set_value(void *me, int16_t value) {
	if (value < this->min_val) value = this->min_val;
	else if (value > this->max_val) value = this->max_val;
	if (value != this->cur_val) uv_ui_refresh(this);
	this->cur_val = value;
}


/// @brief: Sets the minimum value
void uv_uislider_set_min_value(void *me, int16_t min_value) {
	this->min_val = min_value;
	uv_uislider_set_value(this, this->cur_val);
	uv_ui_refresh(this);
}

/// @brief: sets the maximum value
void uv_uislider_set_max_value(void *me, int16_t max_value) {
	this->max_val = max_value;
	uv_uislider_set_value(this, this->cur_val);
	uv_ui_refresh(this);
}




#endif
