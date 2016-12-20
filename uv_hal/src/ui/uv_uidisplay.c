/*
 * uv_display.c
 *
 *  Created on: Sep 22, 2016
 *      Author: usevolt
 */


#include <ui/uv_uidisplay.h>
#include "uv_lcd.h"

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
}



void uv_uidisplay_step(void *me, uint32_t step_ms) {
	// get touch data from the LCD
#if CONFIG_LCD_TOUCHSCREEN
	int16_t x, y, touch;
	touch = uv_lcd_touch_get(&x, &y);
#endif
	uv_touch_st t;


	uv_uiwindow_step(me, &t, step_ms);
}

#endif
