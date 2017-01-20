/*
 * uwindow.c
 *
 *  Created on: Sep 21, 2016
 *      Author: usevolt
 */


#include <ui/uv_uiwindow.h>


#if CONFIG_LCD

#define this	((uv_uiwindow_st*) me)



/// @brief: Redraws this window
static void redraw(void *me) {

	uv_lcd_draw_rect(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_ui_get_bb(this)->width, uv_ui_get_bb(this)->height,
			this->style->window_c);
}

void uv_uiwindow_init(void *me,
		uv_uiobject_st **object_array, const uv_uistyle_st * style) {
	uv_uiobject_init((uv_uiobject_st*) this);
	this->objects = object_array;
	this->objects_count = 0;
	this->style = style;
}


void uv_uiwindow_add(void *me, void *object,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height,
		void (*step_callb)(void*, uv_touch_st*, uint16_t)) {
	uv_uiobject_add(object, (uv_uiobject_st*) this,
			x, y, width, height, step_callb);
	this->objects[this->objects_count++] = object;
}



void uv_uiwindow_step(void *me, uv_touch_st *touch, uint16_t step_ms) {

	if (((uv_uiobject_st*)this)->refresh) {
		// first redraw this window
		redraw(this);
		// then request redraw all children objects
		uint16_t i;
		for (i = 0; i < this->objects_count; i++) {
			uv_ui_refresh_parent(this->objects[i]);
		}
		this->super.refresh = false;
	}
	// call step functions for all children which are visible
	uint16_t i;
	for (i = 0; i < this->objects_count; i++) {
		if (this->objects[i]->visible) {

			// touch event is unique for all children objects
			uv_touch_st t2 = *touch;
			if (t2.action != TOUCH_NONE) {
				if (t2.action != TOUCH_DRAG) {
					t2.x -= uv_uibb(this->objects[i])->x;
					t2.y -= uv_uibb(this->objects[i])->y;
					if (t2.x > uv_uibb(this->objects[i])->width ||
							t2.y > uv_uibb(this->objects[i])->height ||
							t2.x < 0 ||
							t2.y < 0) {
						t2.action = TOUCH_NONE;
					}
				}
			}
			uv_touch_action_e touch_propagate = t2.action;

			this->objects[i]->step_callb(this->objects[i], &t2, step_ms);
			// check if the object changed the touch event.
			// This means that the touch event is processed and it shouldn't
			// be propagating to other objects.
			if (t2.action != touch_propagate) {
				touch->action = TOUCH_NONE;
			}

#if CONFIG_UI_DRAW_BOUDING_BOXES
			if (this->objects[i]->refresh) {
				uv_lcd_draw_frame(uv_ui_get_xglobal(this->objects[i]), uv_ui_get_yglobal(this->objects[i]),
						uv_ui_get_bb(this->objects[i])->width, uv_ui_get_bb(this->objects[i])->height, 1, 0xff0000);
			}
#endif
		}
		// objects cannot set itself to refresh, so disable refresh request
		this->objects[i]->refresh = false;
	}
}

#endif
