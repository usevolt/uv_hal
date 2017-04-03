/*
 * uv_uitogglebutton.c
 *
 *  Created on: Nov 1, 2016
 *      Author: usevolt
 */



#include "ui/uv_uitogglebutton.h"



#if CONFIG_LCD

#define this ((uv_uitogglebutton_st*)me)


static inline void draw(void *me, const uv_bounding_box_st *pbb) {
	color_t bgc = (this->state) ? this->super.style->active_bg_c : this->super.style->inactive_bg_c;
	color_t framec = (this->state) ? this->super.style->active_frame_c : this->super.style->inactive_frame_c;
	color_t fontc = (this->super.state) ? this->super.style->active_font_c : this->super.style->inactive_font_c;
	uv_lcd_draw_mrect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height, bgc,
			pbb->x, pbb->y, pbb->width, pbb->height);

	uv_lcd_draw_mframe(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height, 1, framec,
			pbb->x, pbb->y, pbb->width, pbb->height);

	_uv_ui_draw_mtext(uv_ui_get_xglobal(this) + uv_ui_get_bb(this)->width / 2,
			uv_ui_get_yglobal(this) + uv_ui_get_bb(this)->height / 2,
			this->super.style->font, ALIGN_CENTER, fontc, bgc, this->super.text, 1.0f, pbb);

}


void uv_uitogglebutton_step(void *me, uv_touch_st *touch, uint16_t step_ms, const uv_bounding_box_st *pbb) {
	this->clicked = false;

	if (touch->action == TOUCH_CLICKED) {
		this->clicked = true;
		this->state = !this->state;
		uv_ui_refresh(this);
	}

	if (touch->action != TOUCH_DRAG) {
		// prevent event from propagating to other objects
		touch->action = TOUCH_NONE;
	}

	bool refresh = this->super.super.refresh;
	// make sure that uv_uibutton doesn't trigger drawing the button
	this->super.super.refresh = false;
	// call button step
	uv_uibutton_step(this, touch, step_ms, pbb);

	// update if necessary
	if (refresh) {
		draw(this, pbb);
	}
}




#endif
