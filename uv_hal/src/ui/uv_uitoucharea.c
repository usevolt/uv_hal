/*
 * uv_uitoucharea.c
 *
 *  Created on: Jan 18, 2017
 *      Author: usevolt
 */


#include "ui/uv_uitoucharea.h"


#if CONFIG_LCD

#define this ((uv_uitoucharea_st*)me)


void uv_uitoucharea_init(void *me) {
	uv_uiobject_init(this);
}

void uv_uitoucharea_step(void *me, uv_touch_st *touch, uint16_t step_ms) {
	this->touch = *touch;
	if (touch->action != TOUCH_NONE) {

		// prevent touch action from propagating to other elements
		touch->action = TOUCH_NONE;
	}
}


bool uv_uitoucharea_pressed(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_PRESSED) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_PRESSED;
}

bool uv_uitoucharea_released(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_RELEASED) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_RELEASED;

}

bool uv_uitoucharea_is_down(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_IS_DOWN) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_IS_DOWN;

}

bool uv_uitoucharea_drag(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_DRAG) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_DRAG;

}



#endif

