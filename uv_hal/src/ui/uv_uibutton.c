/*
 * ubutton.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uibutton.h>
#include "ui/uv_uilabel.h"

#define this ((uv_uibutton_st*)me)


static void draw(void *me, uint16_t step_ms) {
	uint8_t scale = 1;
	if (this->state != BUTTON_UP) {
		scale = 2;
	}
	uv_lcd_draw_rect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height,
			uv_cdarker(this->style->main_color, scale));
	uv_lcd_draw_frame(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height,
			this->style->frame_thickness, uv_cdarker(this->style->main_color, scale));

	_uv_ui_draw_text(uv_ui_get_xglobal(this) + uv_ui_get_bb(this)->width / 2,
			uv_ui_get_yglobal(this) + uv_ui_get_bb(this)->height / 2,
			this->style->text_font, ALIGN_CENTER, this->style->text_color, this->text);
}

void uv_uibutton_step(void *me, uv_touch_st *touch, uint16_t step_ms) {

	if (touch->action == TOUCH_CLICKED) {
		this->state = BUTTON_CLICKED;
		this->callb(this, this->state);
	}
	else if (touch->action == TOUCH_LONG_PRESSED) {
		this->state = BUTTON_LONGPRESSED;
		this->callb(this, this->state);
	}
	else {
		this->state = BUTTON_UP;
	}
	if (this->super.refresh) {
		draw(this, step_ms);
	}
}
