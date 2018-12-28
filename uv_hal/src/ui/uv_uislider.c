/*
 * uslider.c
 *
 *  Created on: Aug 19, 2016
 *      Author: usevolt
 */


#include <stdlib.h>
#include <ui/uv_uislider.h>

#if CONFIG_UI

static void draw(void *me, const uv_bounding_box_st *pbb);
static void touch(void *me, uv_touch_st *touch);


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
	this->handle_c = style->active_fg_c;
	this->bg_c = style->inactive_bg_c;
	this->horizontal = true;
	this->show_value = true;
	this->dragging = false;
	this->inc_step = 1;
	this->drag_val = 0;
	this->title = NULL;
	((uv_uiobject_st*) this)->step_callb = &uv_uislider_step;
	uv_uiobject_set_draw_callb(this, &draw);
	uv_uiobject_set_touch_callb(this, &touch);
}


static void draw(void *me, const uv_bounding_box_st *pbb) {
	int16_t x, y, w, h;
	if (this->horizontal) {
		if (uv_uibb(this)->height > CONFIG_UI_SLIDER_WIDTH) {
			uv_uibb(this)->y += uv_uibb(this)->height / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		}
		uv_uibb(this)->height = CONFIG_UI_SLIDER_WIDTH;

		x = uv_ui_get_xglobal(this);
		y = uv_ui_get_yglobal(this) + uv_uibb(this)->height / 2 - CONFIG_UI_SLIDER_WIDTH / 2 -
				((this->title) ? uv_ft81x_get_font_height(this->font) / 2 : 0);
		w = uv_uibb(this)->width;
		h = CONFIG_UI_SLIDER_WIDTH;
		// handle relative position
		int16_t hpx = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hx = uv_lerpi(hpx, 0, uv_uibb(this)->width - CONFIG_UI_SLIDER_WIDTH - 1);

#if CONFIG_LCD
		// draw slider handle
		uv_lcd_draw_mrect(x, y, w, h, this->style->inactive_bg_c,
				pbb);
		uv_lcd_draw_mframe(x, y, w, h, 1, this->style->inactive_frame_c,
				pbb);
		// hx indicates the handle position
		uv_lcd_draw_mrect(x + hx + 1, y + 1, CONFIG_UI_SLIDER_WIDTH - 1, h - 2,
				this->style->active_fg_c, pbb);
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			_uv_ui_draw_mtext(x + w/2, y + h/2, this->style->font, ALIGN_CENTER,
					this->style->inactive_font_c, C(0xFFFFFFFF), str, 1.0f, pbb);
		}
		_uv_ui_draw_mtext(x + 1, y + h / 2, this->style->font, ALIGN_CENTER_LEFT,
				this->style->text_color, C(0xFFFFFFFF), "\x11", 1.0f, pbb);
		_uv_ui_draw_mtext(x + w - 1, y + h / 2, this->style->font, ALIGN_CENTER_RIGHT,
				this->style->text_color, C(0xFFFFFFFF), "\x10", 1.0f, pbb);
#elif CONFIG_FT81X
		uv_ft81x_draw_shadowrrect(x, y + h / 2 - 5, w, 10, CONFIG_UI_RADIUS,
				this->bg_c, uv_uic_brighten(this->bg_c, 30), uv_uic_brighten(this->bg_c, -30));
		// handle
		uv_ft81x_draw_shadowrrect(x + hx, y, CONFIG_UI_SLIDER_WIDTH, h, CONFIG_UI_RADIUS,
				this->handle_c, uv_uic_brighten(this->handle_c, 30), uv_uic_brighten(this->handle_c, -30));
		// handle text
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			uv_ft81x_draw_string(str, this->font,
					x + hx + CONFIG_UI_SLIDER_WIDTH / 2, y + (h / 2),
					ALIGN_CENTER, this->text_c);
		}
#endif
	}
	else {
		if (uv_uibb(this)->width > CONFIG_UI_SLIDER_WIDTH) {
			uv_uibb(this)->x += uv_uibb(this)->width / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		}
		uv_uibb(this)->width = CONFIG_UI_SLIDER_WIDTH;
		x = uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		y = uv_ui_get_yglobal(this);
		w = CONFIG_UI_SLIDER_WIDTH;
		h = uv_uibb(this)->height - (this->title ?
				(uv_ui_text_height_px(this->title, this->font, 1.0f) + 5) : 0);
		// draw slider handle
		// handle relative position
		int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height -
				CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ?
						(uv_ui_text_height_px(this->title, this->font, 1.0f) + 5) : 0), 0);

#if CONFIG_LCD
		uv_lcd_draw_mrect(x, y, w, h, this->style->inactive_bg_c,
				pbb);
		uv_lcd_draw_mframe(x, y, w, h, 1, this->style->inactive_frame_c,
				pbb);
		// hy indicates the handle position
		uv_lcd_draw_mrect(x + 1, y + hy + 1, w - 2, CONFIG_UI_SLIDER_WIDTH - 1,
				this->style->active_fg_c, pbb);
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			_uv_ui_draw_mtext(x + w/2, y + h/2, this->style->font, ALIGN_CENTER,
					this->style->inactive_font_c, C(0xFFFFFFFF), str, 1.0f, pbb);
		}
		_uv_ui_draw_mtext(x + w / 2, y + 1, this->style->font, ALIGN_TOP_CENTER,
				this->style->text_color, C(0xFFFFFFFF), "\x1E", 1.0f, pbb);
		_uv_ui_draw_mtext(x + w / 2, y + h - 1, this->style->font, ALIGN_BOTTOM_CENTER,
				this->style->text_color, C(0xFFFFFFFF), "\x1F", 1.0f, pbb);
#elif CONFIG_FT81X
		uv_ft81x_draw_rrect(x + w / 2 - 7, y, 10, h - 5,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, -30));
		uv_ft81x_draw_rrect(x + 4 + w / 2 - 7, y + 4, 10, h - 5,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->bg_c, 30));
		uv_ft81x_draw_rrect(x + 2 + w / 2 - 7, y + 2, 10, h - 5,
				CONFIG_UI_RADIUS, this->bg_c);
		// handle
		uv_ft81x_draw_rrect(x, y + hy, w - 4, CONFIG_UI_SLIDER_WIDTH - 4,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->handle_c, -30));
		uv_ft81x_draw_rrect(x + 4, y + hy + 4, w - 4, CONFIG_UI_SLIDER_WIDTH - 4,
				CONFIG_UI_RADIUS, uv_uic_brighten(this->handle_c, 30));
		uv_ft81x_draw_rrect(x + 2, y + hy + 2, w - 4, CONFIG_UI_SLIDER_WIDTH - 4,
				CONFIG_UI_RADIUS, this->handle_c);
		// handle text
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			uv_ft81x_draw_string(str, this->font,
					x + (w / 2), y + hy + CONFIG_UI_SLIDER_WIDTH / 2,
					ALIGN_CENTER, this->text_c);
		}
#endif
	}

#if CONFIG_LCD
	_uv_ui_draw_mtext(x + w / 2, y + h + 5, this->style->font, ALIGN_TOP_CENTER,
			this->style->text_color, C(0xFFFFFFFF), this->title, 1.0f, pbb);
#elif CONFIG_FT81X
	uv_ft81x_draw_string(this->title, this->font, x + w / 2, y + h + 5,
			ALIGN_TOP_CENTER, this->text_c);
#endif


}


uv_uiobject_ret_e uv_uislider_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (this->super.refresh && this->super.visible) {
		((uv_uiobject_st*) this)->vrtl_draw(this, pbb);
		this->super.refresh = false;
		ret = UIOBJECT_RETURN_REFRESH;
	}

	return ret;
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
					CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ? this->font->char_height + 5 : 0), 0);
			if (touch->y > hy + CONFIG_UI_SLIDER_WIDTH / 2) {
				i = -this->inc_step;
			}
		}
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
	else if (touch->action == TOUCH_NONE && this->dragging) {
		this->dragging = false;
		this->drag_val = 0;
	}

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
