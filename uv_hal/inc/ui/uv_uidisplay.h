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
#include <uv_filters.h>
#include "uv_utilities.h"

#if CONFIG_UI


#if !defined(CONFIG_UI_CLICK_THRESHOLD)
#error "CONFIG_UI_CLICK_THRESHOLD should define the number of pixels which\
 a touchscreen click event can sustain. If the touch moves more than this\
 number of pixels, touchscreen drag event is triggered."
#endif
#if !defined(CONFIG_UI_TOUCHSCREEN)
#error "CONFIG_UI_TOUCHSCREEN should be defined as 1 or 0 dpending if the UI is \
used on a touchscreen display."
#endif




/// @brief: Defines the number of step cycles used with
/// touch input moving average.
#define UI_TOUCH_AVERAGE_COUNT		1



#if CONFIG_UI

/// @brief: Main display class. This represents a whole display.
typedef struct {
	EXTENDS(uv_uiwindow_st);
#if CONFIG_UI_TOUCHSCREEN
	uv_moving_aver_st avr_x;
	uv_moving_aver_st avr_y;
	/// @brief: Variables holding the press coordinates
	int16_t press_x;
	int16_t press_y;
	/// @brief: Stores the current state of the press event
	int16_t press_state;
#endif
	/// @brief: Optional touch callback which will be called everytime the
	/// user touches the display
	void (*touch_callb)(const uv_touch_st *touch);
} uv_uidisplay_st;


/// @brief: initializes the display
void uv_uidisplay_init(void *me, uv_uiobject_st **objects, const uv_uistyle_st *style);


static inline void uv_uidisplay_set_touch_callb(void *me, void (*touch_callb)(const uv_touch_st *touch)) {
	((uv_uidisplay_st *) me)->touch_callb = touch_callb;
}

/// @brief: Adds an object to the screen
static inline void uv_uidisplay_add(void *me, void *obj,
		uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
	uv_uiwindow_add(me, obj, x, y, width, height);
}


/// @brief: Step function takes care of updating the screen and touch events
void uv_uidisplay_step(void *me, uint32_t step_ms);

#endif

#endif /* UV_HAL_INC_UI_UV_UIDISPLAY_H_ */


#endif
