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

#ifndef UV_HAL_INC_UI_UV_UITOUCHAREA_H_
#define UV_HAL_INC_UI_UV_UITOUCHAREA_H_


#include <ui/uv_uiobject.h>
#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_lcd.h"
#include "ui/uv_ui_styles.h"

/// @file: uv_uitoucharea is an invisible area wich registers the touchscreen actions done
/// over it.

#if CONFIG_UI


typedef struct   {
	EXTENDS(uv_uiobject_st);
	bool transparent;
	uv_touch_st touch;
} uv_uitoucharea_st;


void uv_uitoucharea_init(void *me);



#if defined(this)
#undef this
#endif
#define this ((uv_uitoucharea_st*)me)


/// @brief: Sets the drawing callback function. This
/// will be called every time the toucharea is refreshed.
/// Can be used to draw something on the uitoucharea.
static inline void uv_uitoucharea_set_draw_callb(void *me,
		void (*vrtl_draw)(void *, const uv_bounding_box_st *)) {
	uv_uiobject_set_draw_callb(me, vrtl_draw);
}

/// @brief: Sets if the toucharea is "transparent", e.g.
/// lets the touch events pass through the toucharea to other uiobjects
/// without stopping them. Defaults to false.
static inline void uv_uitoucharea_set_transparent(void *me, bool value) {
	this->transparent = value;
}

static inline bool uv_uitoucharea_get_transparent(void *me) {
	return this->transparent;
}

/// @brief: Returns true if the touch area is pressed. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_pressed(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the touch area is released. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_drag_released(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the touch area is held down. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_is_down(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the touch area was clicked. If *x* and *y* are not NULL,
/// the touch local coordinates are stored to them.
bool uv_uitoucharea_clicked(void *me, int16_t *x, int16_t *y);

/// @brief: Returns true if the toucharea is being dragged. If *x* and *y* are not NULL,
/// the drag movement is stored to them.
bool uv_uitoucharea_is_dragging(void *me, int16_t *x, int16_t *y);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UITOUCHAREA_H_ */
