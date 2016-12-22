/*
 * uv_display.c
 *
 *  Created on: Sep 22, 2016
 *      Author: usevolt
 */


#include <ui/uv_uidisplay.h>
#include "uv_lcd.h"
#include <stdlib.h>

#if CONFIG_LCD

#if CONFIG_LCD_TOUCHSCREEN
enum {
	PRESSING,
	DRAGGING,
	RELEASED
};
#endif



#if CONFIG_LCD

#define this ((uv_uidisplay_st*) me)

void uv_uidisplay_init(void *me, uv_uiobject_st **objects, const uv_uistyle_st *style) {
	uv_uiwindow_init(me, objects, style);
	// display fills the whole screen
	uv_ui_get_bb(me)->x = 0;
	uv_ui_get_bb(me)->y = 0;
	uv_ui_get_bb(me)->width = LCD_W_PX;
	uv_ui_get_bb(me)->height = LCD_H_PX;
	uv_ui_refresh_parent(this);

#if CONFIG_LCD_TOUCHSCREEN
	uv_moving_aver_init(&this->avr_x, UI_TOUCH_AVERAGE_COUNT);
	uv_moving_aver_init(&this->avr_y, UI_TOUCH_AVERAGE_COUNT);
	this->press_state = RELEASED;
#endif
}



void uv_uidisplay_step(void *me, uint32_t step_ms) {
	uv_touch_st t = {};
	t.action = TOUCH_NONE;

	// get touch data from the LCD
#if CONFIG_LCD_TOUCHSCREEN
	bool touch;
	touch = uv_lcd_touch_get(&t.x, &t.y);

	if (touch) {
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
			uv_moving_aver_reset(&this->avr_x);
			uv_moving_aver_reset(&this->avr_y);
			this->press_state = RELEASED;
		}
	}
#endif


	uv_uiwindow_step(me, &t, step_ms);
}

#endif
