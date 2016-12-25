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
					this->style->inactive_font_c, C(0xFFFFFFFF), str);
		}
	}
	else {
		if (uv_uibb(this)->width < CONFIG_UI_SLIDER_WIDTH) {
			this->super.bb.width = CONFIG_UI_SLIDER_WIDTH;
		}
		x = uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		y = uv_ui_get_yglobal(this);
		w = CONFIG_UI_SLIDER_WIDTH;
		h = uv_uibb(this)->height - (this->title ? (this->style->font->char_height + 5) : 0);
		uv_lcd_draw_rect(x, y, w, h, this->style->inactive_bg_c);
		uv_lcd_draw_frame(x, y, w, h, 1,
				this->style->inactive_frame_c);
		// draw slider handle
		// handle relative position
		int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height -
				CONFIG_UI_SLIDER_WIDTH - 1 - (this->title ? this->style->font->char_height + 5 : 0), 0);
		// hx indicates the handle position
		uv_lcd_draw_rect(x + 1, y + hy + 1,
				w - 2, CONFIG_UI_SLIDER_WIDTH - 1,
				this->style->active_fg_c);
		if (this->show_value) {
			char str[10];
			itoa(this->cur_val, str, 10);
			_uv_ui_draw_text(x + w/2, y + h/2, this->style->font, ALIGN_CENTER,
					this->style->inactive_font_c, C(0xFFFFFFFF), str);
		}
	}
	_uv_ui_draw_text(x + w/2, y + h + 5, this->style->font, ALIGN_TOP_CENTER,
			this->style->text_color, C(0xFFFFFFFF), this->title);


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
				(((this->horizontal) ? uv_uibb(this)->width : uv_uibb(this)->height) - CONFIG_UI_SLIDER_WIDTH);
		if (this->cur_val < this->min_val) this->cur_val = this->min_val;
		else if (this->cur_val > this->max_val) this->cur_val = this->max_val;

		// prevent action from propagating into other elements
		touch->action = TOUCH_NONE;
		uv_ui_refresh(this);
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







#endif
