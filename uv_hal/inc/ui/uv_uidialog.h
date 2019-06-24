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


#ifndef UV_HAL_INC_UI_UV_UIDIALOG_H_
#define UV_HAL_INC_UI_UV_UIDIALOG_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"


#if CONFIG_UI

/// @file: UIDialog represents a non-permanent pop-up screen which is shown
/// on top of everything else for a some time period.
/// It has it's own event loop and it handles the same functions as an uidisplay, only
/// for a smaller subsystem.


typedef struct {
	EXTENDS(uv_uidisplay_st);

} uv_uidialog_st;


/// @brief: Initializes the uidialog
void uv_uidialog_init(void *me, uv_uiobject_st **object_array, const uv_uistyle_st* style);


/// @brief: Executes the uidialog. This function returns only when
/// the step function assigned to this uidialog returns UIDIALOG_RETURN_FINISHED.
void uv_uidialog_exec(void *me);


/// @brief: Adds an object into uidialog
static inline void uv_uidialog_addxy(void *me, void *object,
		int16_t x, int16_t y, uint16_t width, uint16_t height) {
	uv_uidisplay_addxy(me, object, x, y, width, height);
}

static inline void uv_uidialog_add(void *me, void *object,
		uv_bounding_box_st *bb) {
	uv_uidisplay_add(me, object, bb);
}


/// @brief: Clears the uidialog from objects
static inline void uv_uidialog_clear(void *me) {
	uv_uidisplay_clear(me);
}


static inline void uv_uidialog_set_transparent(void *me, bool value) {
	uv_uiwindow_set_transparent(me, value);
}


static inline void 	uv_uidialog_set_stepcallback(void *me,
		uv_uiobject_ret_e (*step_callb)(void *, uint16_t), void *user_ptr) {
	uv_uiwindow_set_stepcallback(me, step_callb, user_ptr);
}


#endif

#endif /* UV_HAL_INC_UI_UV_UIDIALOG_H_ */
