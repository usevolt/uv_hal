/*
 * uwindow.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_window.h>


#if CONFIG_LCD



#define this	((uv_window_st*) me)



/// @brief: Redraws this window
static void redraw(void *me) {
	uv_lcd_draw_rect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height,
			this->style->main_color);
	uv_lcd_draw_frame(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height,
			this->style->frame_thickness, this->style->frame_color);
}



void uv_window_step(void *me, uint16_t step_ms) {

	if (this->super.refresh) {
		// first redraw this window
		redraw(this);
		// then request redraw all children objects
		uint16_t i;
		for (i = 0; i < this->objects_count; i++) {
			uv_ui_refresh(&this->objects[i]);
		}
		this->super.refresh = false;
	}

	// call step functions for all children which are visible
	uint16_t i;
	for (i = 0; i < this->objects_count; i++) {
		if (this->objects[i].visible) {
			this->objects[i].step_callb(&this->objects[i], step_ms);
		}
	}
}


#endif
