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


static void draw(void *me) {
	if (this->horizontal) {
		if (uv_uibb(this)->height < CONFIG_UI_SLIDER_WIDTH) {
			this->super.bb.height = CONFIG_UI_SLIDER_WIDTH;
		}
		int16_t x = uv_ui_get_xglobal(this);
		int16_t y = uv_ui_get_yglobal(this) + uv_uibb(this)->height / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		int16_t w = uv_uibb(this)->width;
		int16_t h = CONFIG_UI_SLIDER_WIDTH;
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
		int16_t x = uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2 - CONFIG_UI_SLIDER_WIDTH / 2;
		int16_t y = uv_ui_get_yglobal(this);
		int16_t w = CONFIG_UI_SLIDER_WIDTH;
		int16_t h = uv_uibb(this)->height;
		uv_lcd_draw_rect(x, y, w, h, this->style->inactive_bg_c);
		uv_lcd_draw_frame(x, y, w, h, 1,
				this->style->inactive_frame_c);
		// draw slider handle
		// handle relative position
		int16_t hpy = uv_reli(this->cur_val, this->min_val, this->max_val);
		int16_t hy = uv_lerpi(hpy, uv_uibb(this)->height - CONFIG_UI_SLIDER_WIDTH - 1, 0);
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
