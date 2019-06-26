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

#ifndef UV_HAL_INC_UI_UV_UISLIDER_H_
#define UV_HAL_INC_UI_UV_UISLIDER_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_UI

#if !CONFIG_UI_SLIDER_WIDTH
#error "CONFIG_UI_SLIDER_WIDTH should define the slider *width*. For horizontal slider this means the height.\
 Also handle height is taken from this value."
#endif
#if !CONFIG_UI_SLIDER_INC_DEC_WIDTH
#error "CONFIG_UI_SLIDER_INC_DEC_WIDTH should define the width of uilisder increase and decrease buttons in pizels."
#endif


/// @brief: Slider is a vertical or horizontal slider with a value
typedef struct {
	EXTENDS(uv_uiobject_st);

	int16_t min_val;
	int16_t max_val;
	int16_t cur_val;
	/// @brief: Calculates the dragging value to make the sliding a little bit
	/// more interacive.
	int16_t drag_val;
	int16_t drag_start_val;
	/// @brief: Defines how much the value is changed when tapping the slider. Defualts to 1
	uint16_t inc_step;
	/// @brief: If true, displays the current value next to the slider.
	/// On vertical slider, value is shown below the slider.
	/// On horizontal slider, value is shown to the right of the slider.
	struct {
		/// @brief: If true, the slider is horizontal. If false, the slider is vertical.
		uint8_t show_value : 1;
		/// @brief: Inner state variable indicating that a TOUCH_DRAG event has been started
		uint8_t horizontal : 1;
		uint8_t dragging : 1;
	};
	/// @brief: Title text which is shown below the slider
	char *title;
	/// @brief: Value changed-callback
	void (*callb)(void *me, int16_t value);

	color_t text_c;
	uv_font_st *font;
	color_t handle_c;
	color_t bg_c;
} uv_uislider_st;

#ifdef this
#undef this
#endif
#define this ((uv_uislider_st*)me)


/// @brief: Initializes the slider
///
/// @param min_value: The slider minimum value
/// @param max_value: The maximum value of the slider
/// @param current_value: The initial value of the slider
/// @param callb: The callback which is called when the value is changed. Parameter: pointer to
/// this slider structure, the current value
void uv_uislider_init(void *me, int16_t min_value, int16_t max_value, int16_t current_value,
		const uv_uistyle_st *style);


/// @brief: Step function which is also used to update the slider
uv_uiobject_ret_e uv_uislider_step(void *me, uint16_t step_ms,
		const uv_bounding_box_st *pbb);


/// @brief: Configures the slider as a horizontal slider
static inline void uv_uislider_set_horizontal(void *me) {
	this->horizontal = true;
	uv_ui_refresh(this);
}

/// @brief: Configures the slider as a vertical slider
static inline void uv_uislider_set_vertical(void *me) {
	this->horizontal = false;
	uv_ui_refresh(this);
}

/// @brief: Configures the slider to show current value
static inline void uv_uislider_show_value(void *me) {
	this->show_value = true;
	uv_ui_refresh(this);
}

/// @brief: Configures the slider not to show the current value
static inline void uv_uislider_hide_value(void *me) {
	this->show_value = false;
	uv_ui_refresh(this);
}

static inline void uv_uislider_set_inc_step(void *me, uint16_t value) {
	this->inc_step = value;
}


/// @brief: Sets the minimum value
void uv_uislider_set_min_value(void *me, int16_t min_value);

/// @brief: Sets the title. The title should be a null-terminated string.
static inline void uv_uislider_set_title(void *me, char *title) {
	this->title = title;
}

/// @brief: Returns the minimum value
static inline int16_t uv_uislider_get_min_value(void *me) {
	return this->min_val;
}

/// @brief: sets the maximum value
void uv_uislider_set_max_value(void *me, int16_t max_value);

/// @brief: Returns the maimum value
static inline int16_t uv_uislider_get_max_value(void *me) {
	return this->max_val;
}

/// @brief: Sets the current value
void uv_uislider_set_value(void *me, int16_t value);

/// @brief: Returns the current value
static inline int16_t uv_uislider_get_value(void *me) {
	return this->cur_val;
}

/// @brief: Returns true for one step cycle wen the values was changed
static inline bool uv_uislider_value_changed(void *me) {
	return this->dragging;
}



#undef this
#endif

#endif /* UV_HAL_INC_UI_UV_UISLIDER_H_ */
