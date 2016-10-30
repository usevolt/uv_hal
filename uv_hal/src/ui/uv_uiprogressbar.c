/*
 * uv_uiprogressbar.c
 *
 *  Created on: Oct 30, 2016
 *      Author: usevolt
 */


#include "ui/uv_uiprogressbar.h"


#if CONFIG_LCD


#define this ((uv_uiprogressbar_st *)me)


static void draw(void *me) {
	// total amount of bars to draw
	int16_t bars;
	// amount of active bars
	int16_t active_bars;

	if (this->horizontal) {
		bars = uv_uibb(this)->width /
				(CONFIG_UI_PROGRESSBAR_BAR_WIDTH + CONFIG_UI_PROGRESSBAR_BAR_SPACE);
	}
	else {
		bars = uv_uibb(this)->height /
				(CONFIG_UI_PROGRESSBAR_BAR_WIDTH + CONFIG_UI_PROGRESSBAR_BAR_SPACE);
	}
	int16_t rel = uv_reli(this->value, this->min_val, this->max_val);
	active_bars = uv_lerpi(rel, 0, bars);

	color_t c = ((this->limit_type == UI_PROGRESSBAR_LIMIT_UNDER && this->value < this->limit) ||
			(this->limit_type == UI_PROGRESSBAR_LIMIT_OVER && this->value > this->limit)) ?
			this->limit_color :
			this->style->active_fg_c;
	int16_t x = uv_ui_get_xglobal(this);
	int16_t y = this->horizontal ?
			uv_ui_get_yglobal(this) :
			uv_ui_get_yglobal(this) + uv_uibb(this)->height - CONFIG_UI_PROGRESSBAR_BAR_WIDTH;
	int16_t w = this->horizontal ?
			(CONFIG_UI_PROGRESSBAR_BAR_WIDTH) : uv_uibb(this)->width;
	int16_t h = this->horizontal ?
			uv_uibb(this)->height : CONFIG_UI_PROGRESSBAR_BAR_WIDTH;

	// draw all bars
	for (int16_t i = 0; i < bars; i++) {
		uv_lcd_draw_rect(x, y, w, h, c);
		if (this->horizontal) {
			x += (CONFIG_UI_PROGRESSBAR_BAR_SPACE + CONFIG_UI_PROGRESSBAR_BAR_WIDTH);
		}
		else {
			y -= (CONFIG_UI_PROGRESSBAR_BAR_SPACE + CONFIG_UI_PROGRESSBAR_BAR_WIDTH);
		}
		if (i == active_bars) {
			c = this->style->inactive_bg_c;
		}
	}
}



void uv_uiprogressbar_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	if (this->super.refresh) {
		draw(this);
	}
}


#endif

