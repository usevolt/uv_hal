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

#if !CONFIG_UI_SLIDER_WIDTH
#error "CONFIG_UI_SLIDER_WIDTH should define the slider *width*. For horizontal slider this means the height.\
 Also handle height is taken from this value."
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
	/// @brief: If true, displays the current value next to the slider.
	/// On vertical slider, value is shown below the slider.
	/// On horizontal slider, value is shown to the right of the slider.
	bool show_value;
	/// @brief: If true, the slider is horizontal. If false, the slider is vertical.
	bool horizontal;
	/// @brief: Inner state variable indicating that a TOUCH_DRAG event has been started
	bool dragging;
	/// @brief: Title text which is shown below the slider
	char *title;
	/// @brief: Value changed-callback
	void (*callb)(void *me, int16_t value);
	const uv_uistyle_st *style;
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

/// @brief: Sets the title. The title should be a null-terminated string.
static inline void uv_uislider_set_title(void *me, char *title) {
	this->title = title;
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
	if (value < this->min_val) value = this->min_val;
	else if (value > this->max_val) value = this->max_val;
	if (value != this->cur_val) uv_ui_refresh(this);
	this->cur_val = value;
}

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