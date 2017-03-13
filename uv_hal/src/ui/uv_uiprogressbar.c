/*
 * uv_uiprogressbar.c
 *
 *  Created on: Oct 30, 2016
 *      Author: usevolt
 */


#include "ui/uv_uiprogressbar.h"


#if CONFIG_LCD


#define this ((uv_uiprogressbar_st *)me)


/// @brief: Initializes the progress bar as horizontal bar
void uv_uiprogressbar_init(void *me, int16_t min_value,
		int16_t max_value, const uv_uistyle_st *style) {
	uv_uiobject_init(this);
	this->min_val = min_value;
	this->max_val = max_value;
	this->style = style;
	this->horizontal = true;
	this->value = this->min_val;
	this->limit = this->min_val;
	this->limit_type = UI_PROGRESSBAR_LIMIT_NONE;
	this->title = NULL;
}


void uv_uiprogressbar_set_value(void *me, int16_t value) {
	if (value > this->max_val) value = this->max_val;
	else if (value < this->min_val) value = this->min_val;
	if (this->value != value) {
		uv_ui_refresh(this);
		this->value = value;
	}
}


void uv_uiprogressbar_set_limit(void *me, uiprogressbar_limit_e type,
		int16_t limit, color_t color) {
	this->limit_type = type;
	this->limit = limit;
	this->limit_color = color;
	uv_ui_refresh(this);
}


static void draw(void *me) {
	// total amount of bars to draw
	int16_t bars;
	// amount of active bars
	int16_t active_bars;

	if (this->horizontal) {
		bars = uv_uibb(this)->width /
				(CONFIG_UI_PROGRESSBAR_WIDTH + CONFIG_UI_PROGRESSBAR_SPACE);
	}
	else {
		bars = (uv_uibb(this)->height -
				(this->title ? uv_ui_text_height_px(this->title, this->style->font, 1.0f) : 0)) /
				(CONFIG_UI_PROGRESSBAR_WIDTH + CONFIG_UI_PROGRESSBAR_SPACE) - 1;
	}
	int16_t rel = uv_reli(this->value, this->min_val, this->max_val);
	if (rel < 0) { rel = 0; }
	active_bars = uv_lerpi(rel, 0, bars);


	color_t c = ((this->limit_type == UI_PROGRESSBAR_LIMIT_UNDER && this->value < this->limit) ||
			(this->limit_type == UI_PROGRESSBAR_LIMIT_OVER && this->value > this->limit)) ?
			this->limit_color :
			this->style->active_fg_c;
	int16_t x = this->horizontal ?
			uv_ui_get_xglobal(this) :
			uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2 - CONFIG_UI_PROGRESSBAR_HEIGHT / 2;
	int16_t y = this->horizontal ?
			uv_ui_get_yglobal(this) + uv_uibb(this)->height / 2 - CONFIG_UI_PROGRESSBAR_HEIGHT / 2 :
			uv_ui_get_yglobal(this) + uv_uibb(this)->height -
			(this->title ? (uv_ui_text_height_px(this->title, this->style->font, 1.0f) + 3) : 0) -
			CONFIG_UI_PROGRESSBAR_WIDTH;
	int16_t w = this->horizontal ?
			(CONFIG_UI_PROGRESSBAR_WIDTH) : CONFIG_UI_PROGRESSBAR_HEIGHT;
	int16_t h = this->horizontal ?
			CONFIG_UI_PROGRESSBAR_HEIGHT : CONFIG_UI_PROGRESSBAR_WIDTH;

	// draw all bars
	for (int16_t i = 0; i < bars; i++) {
		uv_lcd_draw_rect(x, y, w, h, c);
		if (this->horizontal) {
			x += (CONFIG_UI_PROGRESSBAR_SPACE + CONFIG_UI_PROGRESSBAR_WIDTH);
		}
		else {
			y -= (CONFIG_UI_PROGRESSBAR_SPACE + CONFIG_UI_PROGRESSBAR_WIDTH);
		}
		if (i == active_bars) {
			c = this->style->inactive_bg_c;
		}
	}
	// draw title
	if (this->title) {
		_uv_ui_draw_text(uv_ui_get_xglobal(this) + uv_uibb(this)->width / 2,
				uv_ui_get_yglobal(this) + uv_uibb(this)->height,
				this->style->font, ALIGN_BOTTOM_CENTER,
				this->style->text_color, C(0xFFFFFFFF), this->title, 1.0f);
	}
}



void uv_uiprogressbar_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	if (this->super.refresh) {
		draw(this);
	}
}


#endif

