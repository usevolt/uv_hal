/*
 * uslider.h
 *
 *  Created on: Aug 19, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UISLIDER_H_
#define UV_HAL_INC_UI_UV_UISLIDER_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_ui.h"

#if CONFIG_LCD


/// @brief: Slider is a vertical or horizontal slider with a value
typedef struct {
	EXTENDS(uv_uiobject_st);

	int16_t min_val;
	int16_t max_val;
	int16_t cur_val;
	/// @brief: If true, displays the current value next to the slider.
	/// On vertical slider, value is shown below the slider.
	/// On horizontal slider, value is shown to the right of the slider.
	bool show_value;
	/// @brief: If true, the slider is horizontal. If false, the slider is vertical.
	bool horizontal;
	/// @brief: Value changed-callback
	void (*callb)(void *me, int16_t value);
	const uv_uislider_style_st *style;
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
static inline void uv_uislider_init(void *me, int16_t min_value, int16_t max_value, int16_t current_value,
		const uv_uislider_style_st *style, void (*callb)(void *, int16_t)) {
	uv_uiobject_init(this);
	this->min_val = min_value;
	this->max_val = max_value;
	this->cur_val = current_value;
	this->callb = callb;
	this->style = style;
	this->horizontal = true;
	this->show_value = true;
}


/// @brief: Step function which is also used to update the slider
void uv_uislider_step(void *me, uv_touch_st *touch, uint16_t step_ms);


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


/// @brief: Sets the minimum value
static inline void uv_uislider_set_min_value(void *me, int16_t min_value) {
	this->min_val = min_value;
	uv_ui_refresh(this);
}

/// @brief: Returns the minimum value
static inline int16_t uv_uislider_get_min_value(void *me) {
	return this->min_val;
}

/// @brief: sets the maximum value
static inline void uv_uislider_set_max_value(void *me, int16_t max_value) {
	this->max_val = max_value;
	uv_ui_refresh(this);
}

/// @brief: Returns the maimum value
static inline int16_t uv_uislider_get_max_value(void *me) {
	return this->max_val;
}

/// @brief: Sets the current value
static inline void uv_uislider_set_value(void *me, int16_t value) {
	this->cur_val = value;
	uv_ui_refresh(this);
}

/// @brief: Returns the current value
static inline int16_t uv_uislider_get_value(void *me) {
	return this->cur_val;
}



#undef this
#endif

#endif /* UV_HAL_INC_UI_UV_UISLIDER_H_ */
