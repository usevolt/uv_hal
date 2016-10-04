/*
 * uv_display.h
 *
 *  Created on: Sep 22, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_DISPLAY_H_
#define UV_HAL_INC_UI_UV_DISPLAY_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_window.h"


#if CONFIG_LCD

/// @brief: Main display class. This represents a whole display.
typedef struct {
	EXTENDS(uv_window_st);



} uv_display_st;


/// @brief: initializes the display
void uv_display_init(void *me, uv_ui_object_st *objects, uv_window_style_st *style);



static inline void uv_display_add(void *me, uv_window_st *window,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height,
		bool visible, void (*step_callb)(void*, uint16_t)) {
	uv_window_add(me, window, x, y, width, height, visible, step_callb);
}


#endif

#endif /* UV_HAL_INC_UI_UV_DISPLAY_H_ */
