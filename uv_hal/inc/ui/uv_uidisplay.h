/*
 * uv_display.h
 *
 *  Created on: Sep 22, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIDISPLAY_H_
#define UV_HAL_INC_UI_UV_UIDISPLAY_H_


#include <ui/uv_uiwindow.h>
#include <uv_hal_config.h>
#include "uv_utilities.h"


/// @brief: Main display class. This represents a whole display.
typedef struct {
	EXTENDS(uv_uiwindow_st);



} uv_uidisplay_st;


/// @brief: initializes the display
void uv_uidisplay_init(void *me, uv_uiobject_st **objects, const uv_uiwindow_style_st *style);



/// @brief: Adds a window object to th screen
static inline void uv_uidisplay_add(void *me, uv_uiwindow_st *window,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height,
		bool visible, void (*step_callb)(void*, uv_touch_st*, uint16_t)) {
	uv_uiwindow_add(me, window, x, y, width, height, visible, step_callb);
}


/// @brief: Step function takes care of updating the screen and touch events
void uv_uidisplay_step(void *me, uint32_t step_ms);


#endif /* UV_HAL_INC_UI_UV_UIDISPLAY_H_ */
