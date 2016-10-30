/*
 * ubutton.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uibutton.h>
#include "ui/uv_uilabel.h"

#define this ((uv_uibutton_st*)me)


static inline void draw(void *me, uint16_t step_ms) {
	color_t bgc = (this->state) ? this->style->active_bg_c : this->style->inactive_bg_c;
	color_t framec = (this->state) ? this->style->active_frame_c : this->style->inactive_frame_c;
	color_t fontc = (this->state) ? this->style->active_font_c : this->style->inactive_font_c;
	uv_lcd_draw_rect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height, bgc);

	uv_lcd_draw_frame(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height, 1, framec);

	_uv_ui_draw_text(uv_ui_get_xglobal(this) + uv_ui_get_bb(this)->width / 2,
			uv_ui_get_yglobal(this) + uv_ui_get_bb(this)->height / 2,
			this->style->font, ALIGN_CENTER, fontc, bgc, this->text);
}

void uv_uibutton_step(void *me, uv_touch_st *touch, uint16_t step_ms) {

	if (touch->action == TOUCH_CLICKED) {
		this->state = UIBUTTON_CLICKED;
		this->callb(this, this->state);
	}
	else if (touch->action == TOUCH_LONG_PRESSED) {
		this->state = UIBUTTON_LONGPRESSED;
		this->callb(this, this->state);
	}
	else {
		this->state = UIBUTTON_UP;
	}
	if (this->super.refresh) {
		draw(this, step_ms);
	}
}
