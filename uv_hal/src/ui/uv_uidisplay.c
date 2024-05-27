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


#include <ui/uv_uidisplay.h>
#include "uv_lcd.h"
#include <stdlib.h>

#if CONFIG_UI

#if CONFIG_UI_TOUCHSCREEN
enum {
	PRESSING,
	DRAGGING,
	RELEASED
};
#endif



#define this ((uv_uidisplay_st*) me)


static void _uv_uidisplay_draw(void *me, const uv_bounding_box_st *pbb);


void uv_uidisplay_draw(void *me) {
	_uv_uidisplay_draw(this, uv_uibb(this));
}


static void _uv_uidisplay_draw(void *me, const uv_bounding_box_st *pbb) {
	uv_ui_set_mask(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_uibb(this)->width, uv_uibb(this)->height);

	uv_ui_clear(this->display_c);
	uv_ui_draw_point(LCD_WPPT(500), -400, uv_uic_brighten(this->display_c, 20), 1000);

	// draw all the objects added to the screen
	_uv_uiwindow_draw_children(this, pbb);

	// all UI components should now be updated, swap display list buffers
	uv_ui_dlswap();


}


void uv_uidisplay_init(void *me, uv_uiobject_st **objects, const uv_uistyle_st *style) {
	uv_uiwindow_init(me, objects, style);
	// display fills the whole screen
	uv_uibb(me)->x = 0;
	uv_uibb(me)->y = 0;
	uv_uibb(me)->width = LCD_W_PX;
	uv_uibb(me)->height = LCD_H_PX;
	this->display_c = style->display_c;
	uv_ui_refresh_parent(this);
	uv_uiobject_set_draw_callb(this, &_uv_uidisplay_draw);
	// have to set touch callback to null as uiwindow tries to set it to itself
	uv_uiobject_set_touch_callb(this, NULL);

#if CONFIG_UI_TOUCHSCREEN
	uv_delay_init(&this->press_delay, UIDISPLAY_PRESS_DELAY_MS);
	uv_moving_aver_init(&this->avr_x, UI_TOUCH_AVERAGE_COUNT);
	uv_moving_aver_init(&this->avr_y, UI_TOUCH_AVERAGE_COUNT);
	this->press_state = RELEASED;
#endif
}



#define DRAG_MAX_SPEED_PX		50

uv_uiobject_ret_e uv_uidisplay_step(void *me, uint32_t step_ms) {
	uv_uiobject_ret_e ret;
	uv_touch_st t = {};
	t.action = TOUCH_NONE;

	// get touch data from the LCD
#if CONFIG_UI_TOUCHSCREEN
	bool touch;
	touch = uv_ui_get_touch(&t.x, &t.y);

	uv_delay(&this->press_delay, step_ms);

	if (touch &&
			uv_delay_has_ended(&this->press_delay)) {
		t.x = uv_moving_aver_step(&this->avr_x, t.x);
		t.y = uv_moving_aver_step(&this->avr_y, t.y);
		if (this->press_state == RELEASED) {
			this->press_x = t.x;
			this->press_y = t.y;
			t.action = TOUCH_PRESSED;
			this->press_state = PRESSING;
		}
		else if (this->press_state == PRESSING) {
			if (abs(this->press_x - t.x) > CONFIG_UI_CLICK_THRESHOLD ||
					abs(this->press_y - t.y) > CONFIG_UI_CLICK_THRESHOLD) {
				t.action = TOUCH_DRAG;
				this->drag_x = t.x;
				this->drag_y = t.y;
				t.x = 0;
				t.y = 0;
				this->press_state = DRAGGING;
			}
			else {
				t.action = TOUCH_IS_DOWN;
				t.x = this->press_x;
				t.y = this->press_y;
			}
		}
		else if (this->press_state == DRAGGING) {
			// store the current position
			int16_t tx = t.x, ty = t.y;
			t.action = TOUCH_DRAG;
			// drag event gives an offset from
			// last drag position as parameters
			t.x -= this->drag_x;
			t.y -= this->drag_y;
			if (abs(t.x) > DRAG_MAX_SPEED_PX ||
					abs(t.y) > DRAG_MAX_SPEED_PX) {
				// dragging speed exceeded the maximum allowed dragging speed.
				// This might indicate a faulty press, thus we ignore the dragging
				// on this step cycle
				t.x = 0;
				t.y = 0;
			}
			// save current position to drag variables
			this->drag_x = tx;
			this->drag_y = ty;
		}
	}
	else {
		if (this->press_state != RELEASED) {
			t.x = this->press_x;
			t.y = this->press_y;
			if (this->press_state == PRESSING) {
				t.action = TOUCH_CLICKED;
			}
			else {
				t.action = TOUCH_RELEASED;
			}
			uv_delay_init(&this->press_delay, UIDISPLAY_PRESS_DELAY_MS);
			uv_moving_aver_reset(&this->avr_x);
			uv_moving_aver_reset(&this->avr_y);
			this->press_state = RELEASED;
		}
	}
#endif
	// call user touch callback
	if ((((uv_uiobject_st*) this)->vrtl_touch) && (t.action != TOUCH_NONE)) {
		((uv_uiobject_st*) this)->vrtl_touch(this, &t);
	}


	// call uiwindow's touch callback to
	// propagate touches to all objects in reverse order
	_uv_uiwindow_touch(this, &t);

	// call the step function and let it propagate through all objects in order
	ret = uv_uiwindow_step(me, step_ms);


	if (ret & UIOBJECT_RETURN_KILLED) {
		// if UIOBJECT_RETURN_KILLED was returned, the children of objects might
		// have changed and some might not have been called. This can create glitches
		// on the display, thus request refresh of the whole screen
		uv_ui_refresh(this);
	}

	// if refreshing was requested in the step functions, draw the whole screen
	if (((uv_uiobject_st *) this)->refresh || ret == UIOBJECT_RETURN_REFRESH ||
			uv_ui_get_refresh_request()) {
		((uv_uiobject_st*) this)->refresh = true;
		_uv_uiobject_draw(this, uv_uibb(this));
	}

	return ret;
}



void uv_uidisplay_clear(void *me) {
	uv_uiwindow_clear(me);
	uv_uiobject_set_draw_callb(me, &_uv_uidisplay_draw);
}




#endif
