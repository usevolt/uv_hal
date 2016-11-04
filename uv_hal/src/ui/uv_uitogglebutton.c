/*
 * uv_uitogglebutton.c
 *
 *  Created on: Nov 1, 2016
 *      Author: usevolt
 */



#include "ui/uv_uitogglebutton.h"



#if CONFIG_LCD

#define this ((uv_uitogglebutton_st*)me)


static inline void draw(void *me) {
	color_t bgc = (this->state) ? this->super.style->active_bg_c : this->super.style->inactive_bg_c;
	color_t framec = (this->state) ? this->super.style->active_frame_c : this->super.style->inactive_frame_c;
	color_t fontc = (this->super.state) ? this->super.style->active_font_c : this->super.style->inactive_font_c;
	uv_lcd_draw_rect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height, bgc);

	uv_lcd_draw_frame(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height, 1, framec);

	_uv_ui_draw_text(uv_ui_get_xglobal(this) + uv_ui_get_bb(this)->width / 2,
			uv_ui_get_yglobal(this) + uv_ui_get_bb(this)->height / 2,
			this->super.style->font, ALIGN_CENTER, fontc, bgc, this->super.text);

}


void uv_uitogglebutton_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	bool refresh = this->super.super.refresh;

	// make sure that uv_uibutton doesnt trigger drawing the button
	this->super.super.refresh = false;

	if (touch->action == TOUCH_CLICKED || touch->action == TOUCH_RELEASED) {
		this->state = !this->state;
	}

	// call button step
	uv_uibutton_step(this, touch, step_ms);

	// update if necessary
	if (refresh) {
		draw(this);
	}
}




#endif
