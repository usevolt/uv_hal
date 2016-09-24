/*
 * uv_display.c
 *
 *  Created on: Sep 22, 2016
 *      Author: usevolt
 */


#include "ui/uv_display.h"
#include "uv_lcd.h"

#define this ((uv_display_st*) me)

void uv_display_init(void *me, uv_ui_object_st *objects, uv_window_style_st *style) {
	uv_window_init(me, objects, style);
	// display fills the whole screen
	uv_ui_get_bb(me)->x = 0;
	uv_ui_get_bb(me)->y = 0;
	uv_ui_get_bb(me)->width = LCD_W_PX;
	uv_ui_get_bb(me)->height = LCD_H_PX;
}


