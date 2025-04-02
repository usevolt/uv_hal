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

#define UIDISPLAY_PRESS_DELAY_MS	100

/// @brief: Main display class. This represents a whole display.
typedef struct {
	EXTENDS(uv_uiwindow_st);
#if CONFIG_UI_TOUCHSCREEN
	uv_moving_aver_st avr_x;
	uv_moving_aver_st avr_y;
	/// @brief: Variables holding the press coordinates
	int16_t press_x;
	int16_t press_y;
	int16_t drag_x;
	int16_t drag_y;
	/// @brief: Stores the current state of the press event
	int16_t press_state;

	// small delay so that two presses cannot happen directly after one another
	uv_delay_st press_delay;

	color_t display_c;
	// when true, small dot indicates touch position on screen
	bool touch_ind;
	uv_delay_st touch_ind_delay;
	uv_touch_st touch;
#endif
} uv_uidisplay_st;


/// @brief: initializes the display
void uv_uidisplay_init(void *me, uv_uiobject_st **objects, const uv_uistyle_st *style);


#if !defined(this)
#define this ((uv_uidisplay_st*) me)
#endif

/// @brief: For uidisplay, uiobejct's virtual touch function is used as a user touch callback
static inline void uv_uidisplay_set_touch_callb(void *me,
		void (*touch_callb)(void *, uv_touch_st *)) {
	uv_uiobject_set_touch_callb(me, touch_callb);
}

/// @brief Sets the display background color
static inline void uv_uidisplay_set_color(void *me, color_t c) {
	this->display_c = c;
}

static inline color_t uv_uidisplay_get_color(void *me) {
	return this->display_c;
}


/// @brief: Adds an object to the screen
static inline void uv_uidisplay_addxy(void *me, void *obj,
		int16_t x, int16_t y, uint16_t width, uint16_t height) {
	uv_uiwindow_addxy(me, obj, x, y, width, height);
}

static inline void uv_uidisplay_add(void *me, void *obj,
		uv_bounding_box_st *bb) {
	uv_uiwindow_add(me, obj, bb);
}

/// @brief: Removes an object from the screen
static inline void uv_uidisplay_remove(void *me, void *obj) {
	uv_uiwindow_remove(me, obj);
}

void uv_uidisplay_clear(void *me);

/// @brief: Step function takes care of updating the screen and touch events
uv_uiobject_ret_e uv_uidisplay_step(void *me, uint32_t step_ms);


/// @brief: Refreshes the screen and redraw all children instantly.
/// This is automatically called after every step cycle, but can also
/// be called anytime inside the gui step task.
void uv_uidisplay_draw(void *me);

void uv_uidisplay_draw_touch_ind(void *me);


static inline void uv_uidisplay_set_touch_indicator(void *me, bool value) {
	this->touch_ind = value;
}

static inline bool uv_uidisplay_get_touch_indicator(void *me) {
	return this->touch_ind;
}


/// @brief: Returns pointer to the touch struct
static inline uv_touch_st *uv_uidisplay_get_touch(void *me) {
	return &this->touch;
}


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIDISPLAY_H_ */


#endif
