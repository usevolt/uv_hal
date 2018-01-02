/*
 * uv_display.c
 *
 *  Created on: Sep 22, 2016
 *      Author: usevolt
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

	uv_ft81x_clear(((uv_uiwindow_st*) this)->style->display_c);
	uv_ft81x_draw_point(LCD_W(0.5f), -400, ((uv_uiwindow_st*) this)->style->active_bg_c, 1000);
#endif
}


void uv_uidisplay_init(void *me, uv_uiobject_st **objects, const uv_uistyle_st *style) {
	uv_uiwindow_init(me, objects, style);
	// display fills the whole screen
	uv_uibb(me)->x = 0;
	uv_uibb(me)->y = 0;
	uv_uibb(me)->width = LCD_W_PX;
	uv_uibb(me)->height = LCD_H_PX;
	uv_ui_refresh_parent(this);
	uv_uiobject_set_draw_callb(this, &draw);
	// ave to set touch callback to null as uiwindow tries to set it to itself
	uv_uiobject_set_touch_callb(this, NULL);

#if CONFIG_UI_TOUCHSCREEN
	uv_moving_aver_init(&this->avr_x, UI_TOUCH_AVERAGE_COUNT);
	uv_moving_aver_init(&this->avr_y, UI_TOUCH_AVERAGE_COUNT);
	this->press_state = RELEASED;
#endif
}



uv_uiobject_ret_e uv_uidisplay_step(void *me, uint32_t step_ms) {
	uv_touch_st t = {};
	t.action = TOUCH_NONE;

	// get touch data from the LCD
#if CONFIG_UI_TOUCHSCREEN
	bool touch;
#if CONFIG_LCD
	touch = uv_lcd_touch_get(&t.x, &t.y);
#elif CONFIG_FT81X
	touch = uv_ft81x_get_touch(&t.x, &t.y);
#endif

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
	// call user touch callback
	if ((((uv_uiobject_st*) this)->vrtl_touch) && (t.action != TOUCH_NONE)) {
		((uv_uiobject_st*) this)->vrtl_touch(this, &t);
	}

	// call uiwindow's touch callback to
	// propagate touches to all objects in reverse order
	uv_uiwindow_touch_callb(this, &t);

	uv_uiobject_ret_e ret = uv_uiwindow_step(me, step_ms, uv_uibb(this));
	if (ret != UIOBJECT_RETURN_ALIVE) {

#if CONFIG_LCD
		// if lcd is configured to use double buffering,
		// swap buffers since all UI components should now be updated
		uv_lcd_double_buffer_swap();
#elif CONFIG_FT81X
		// all UI components should now be updated, swap display list buffers
		uv_ft81x_dlswap();
#endif
	}

	return ret;
}



#endif
