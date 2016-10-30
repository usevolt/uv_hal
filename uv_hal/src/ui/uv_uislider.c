/*
 * uslider.c
 *
 *  Created on: Aug 19, 2016
 *      Author: usevolt
 */


#include <stdlib.h>
#include <ui/uv_uislider.h>
#include "ui/ugui.h"
#if CONFIG_LCD

#define this ((uv_uislider_st*) me)


static void draw(void *me) {
	if (this->horizontal) {
		if (uv_uibb(this)->height < CONFIG_UI_SLIDER_WIDTH) {
			this->super.bb.height = CONFIG_UI_SLIDER_WIDTH;
		}
		int16_t x = uv_ui_get_xglobal(this);
		int16_t y = uv_ui_get_yglobal(this);
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
		int16_t x = uv_ui_get_xglobal(this);
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
	if (touch->action) {

	}
	if (this->super.refresh && this->super.visible) {
		draw(this);
	}
}



#endif
