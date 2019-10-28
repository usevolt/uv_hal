/*
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 *
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "ui/uv_uitoucharea.h"


#if CONFIG_UI

#define this ((uv_uitoucharea_st*)me)


static void touch(void *me, uv_touch_st* touch);

void uv_uitoucharea_init(void *me) {
	uv_uiobject_init(this);
	uv_uiobject_set_touch_callb(this, &touch);
	this->touch.action = TOUCH_NONE;
	this->transparent = false;
}



static void touch(void *me, uv_touch_st* touch) {
	if (!this->super.enabled) {
		this->touch.action = TOUCH_NONE;
	}
	else {
		this->touch = *touch;
		if (!this->transparent &&
				touch->action != TOUCH_NONE &&
				touch->action != TOUCH_DRAG) {

			// prevent touch action from propagating to other elements
			touch->action = TOUCH_NONE;
		}
	}
}


bool uv_uitoucharea_pressed(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_PRESSED) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_PRESSED;
}

bool uv_uitoucharea_drag_released(void *me, int16_t *x, int16_t *y) {
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


bool uv_uitoucharea_is_dragging(void *me, int16_t *x, int16_t *y) {
	if (x && y && this->touch.action == TOUCH_DRAG) {
		*x = this->touch.x;
		*y = this->touch.y;
	}
	return this->touch.action == TOUCH_DRAG;
}



#endif

