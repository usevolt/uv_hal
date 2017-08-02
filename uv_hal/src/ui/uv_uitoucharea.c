/*
 * uv_uitoucharea.c
 *
 *  Created on: Jan 18, 2017
 *      Author: usevolt
 */


#include "ui/uv_uitoucharea.h"


#if CONFIG_UI

#define this ((uv_uitoucharea_st*)me)


void uv_uitoucharea_init(void *me) {
	uv_uiobject_init(this);
	((uv_uiobject_st*) this)->step_callb = &uv_uitoucharea_step;
	this->draw_callb = NULL;
}

uv_uiobject_ret_e uv_uitoucharea_step(void *me, uv_touch_st *touch,
		uint16_t step_ms, const uv_bounding_box_st *pbb) {
	uv_uiobject_ret_e ret = UIOBJECT_RETURN_ALIVE;

	if (this->super.refresh) {
		if (this->draw_callb) {
			this->draw_callb();
		}
		this->super.refresh = false;
	}
	if (!this->super.enabled) {
		this->touch.action = TOUCH_NONE;
	}
	else {
		this->touch = *touch;
		if (touch->action != TOUCH_NONE && touch->action != TOUCH_DRAG) {

			// prevent touch action from propagating to other elements
			touch->action = TOUCH_NONE;
		}
	}
	return ret;
}


void uv_uitoucharea_set_draw_callb(void *me, void (*callb)(void)) {
	this->draw_callb = callb;
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

bool uv_uitoucharea_clicked(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_CLICKED) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_CLICKED;

}



#endif

