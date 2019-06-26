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


static void draw(void *me, const uv_bounding_box_st *pbb);
extern void uv_uiwindow_touch_callb(void *me, uv_touch_st *touch);



static void draw(void *me, const uv_bounding_box_st *pbb) {
#if CONFIG_LCD
	uv_lcd_draw_rect(uv_uibb(this)->x, uv_uibb(this)->y, uv_uibb(this)->width,
			uv_uibb(this)->height, ((uv_uiwindow_st*) this)->style->window_c);
#elif CONFIG_FT81X
	uv_ft81x_set_mask(uv_ui_get_xglobal(this), uv_ui_get_yglobal(this),
			uv_uibb(this)->width, uv_uibb(this)->height);

	uv_ft81x_clear(this->display_c);
	uv_ft81x_draw_point(LCD_W(0.5f), -400, uv_uic_brighten(this->display_c, 20), 1000);
#endif
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
	uv_uiobject_set_draw_callb(this, &draw);
	// ave to set touch callback to null as uiwindow tries to set it to itself
	uv_uiobject_set_touch_callb(this, NULL);

#if CONFIG_UI_TOUCHSCREEN
	uv_delay_init(&this->press_delay, UIDISPLAY_PRESS_DELAY_MS);
	uv_moving_aver_init(&this->avr_x, UI_TOUCH_AVERAGE_COUNT);
	uv_moving_aver_init(&this->avr_y, UI_TOUCH_AVERAGE_COUNT);
	this->press_state = RELEASED;
#endif
}



uv_uiobject_ret_e uv_uidisplay_step(void *me, uint32_t step_ms) {
	uv_uiobject_ret_e ret;
	uv_touch_st t = {};
	t.action = TOUCH_NONE;

	// get touch data from the LCD
#if CONFIG_UI_TOUCHSCREEN
	bool touch;
	touch = uv_ft81x_get_touch(&t.x, &t.y);

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
				int16_t tx = t.x, ty = t.y;
				t.x -= this->press_x;
				t.y -= this->press_y;
				this->press_x = tx;
				this->press_y = ty;
				this->press_state = DRAGGING;
			}
			else {
				t.action = TOUCH_IS_DOWN;
			}
		}
		else if (this->press_state == DRAGGING) {
			// store the current position
			int16_t tx = t.x, ty = t.y;
			t.action = TOUCH_DRAG;
			// drag event gives an offset from
			// last drag position as parameters
			t.x -= this->press_x;
			t.y -= this->press_y;
			// save current position to touch variables
			this->press_x = tx;
			this->press_y = ty;
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
	uv_uiwindow_touch_callb(this, &t);

	ret = uv_uiwindow_step(me, step_ms, uv_uibb(this));
	if (ret & UIOBJECT_RETURN_KILLED) {
		// if UIOBJECT_RETURN_KILLED was returned, the children of objects might
		// have changed and some might not have been called. This can create glitches
		// on the display, thus call step function once more
		uv_ui_refresh(this);
	}

	if (ret == UIOBJECT_RETURN_REFRESH) {
		// all UI components should now be updated, swap display list buffers
		uv_ft81x_dlswap();
	}

	return ret;
}



#endif
