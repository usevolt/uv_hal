/*
 * uv_uiprogressbar.h
 *
 *  Created on: Oct 30, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_
#define UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_


#include <uv_hal_config.h>
#include <uv_ui.h>
#include <uv_utilities.h>


#if CONFIG_LCD

#if !CONFIG_UI_PROGRESSBAR_BAR_WIDTH
#error "CONFIG_UI_PROGRESSBAR_BAR_WIDTH should define the width of individual\
 progressbar's bars in pixels."
#endif
#if !CONFIG_UI_PROGRESSBAR_BAR_SPACE
#error "CONFIG_UI_PROGRESSBAR_BAR_SPACE should define the space between individual\
 progressbar's bars in pixels."
#endif


enum {
	UI_PROGRESSBAR_LIMIT_NONE = 0,
	UI_PROGRESSBAR_LIMIT_OVER,
	UI_PROGRESSBAR_LIMIT_UNDER
};
typedef uint8_t uiprogressbar_limit_e;

typedef struct {
	EXTENDS(uv_uiobject_st);

	/// @brief: Current value
	int16_t value;
	/// @brief: Minimum (zero) value
	int16_t min_val;
	/// @brief: Maximum value
	int16_t max_val;
	/// @brief: If set to true, this progress bar will be shown horizontally.
	/// Default to true.
	bool horizontal;
	/// @brief: Specifies if the limit_color is shown when the value
	/// is over or under the limit value
	uiprogressbar_limit_e limit_type;
	/// @brief: When the value is below or over this, active bar color is changed to *low_color*
	int16_t limit;
	/// @brief: Optional second color which is shown when the value is below *limit*
	color_t limit_color;

	const uv_uistyle_st *style;
} uv_uiprogressbar_st;

#ifdef this
#undef this
#endif
#define this ((uv_uiprogressbar_st *)me)

/// @brief: Initializes the progress bar as horizontal bar
static inline void uv_uiprogressbar_init(void *me, const uv_uistyle_st *style,
		int16_t min_value, int16_t max_value) {
	uv_uiobject_init(this);
	this->min_val = min_value;
	this->max_val = max_value;
	this->style = style;
	this->horizontal = true;
	this->value = this->min_val;
	this->limit = this->min_val;
	this->limit_type = UI_PROGRESSBAR_LIMIT_NONE;
}

/// @brief: Displays the progressbar as horizontal. This is the default.
static inline void uv_uiprogressbar_set_horizontal(void *me) {
	this->horizontal = true;
	uv_ui_refresh_parent(this);
}

/// @brief: Displays the progressbar as vertical
static inline void uv_uiprogressbar_set_vertical(void *me) {
	this->horizontal = false;
	uv_ui_refresh_parent(this);
}

/// @brief: Sets the progressbar current value.
///
/// @param value: Value from 0 to 1000, e.g. part-per-thousands
static inline void uv_uiprogressbar_value(void *me, uint16_t value) {
	if (value > this->max_val) value = this->max_val;
	else if (value < this->min_val) value = this->min_val;
	if (this->value != value) {
		uv_ui_refresh(this);
		this->value = value;
	}
}

/// @brief: Sets the limit color. When the value is below *limit*,
/// active bar color will be changed to *color*.
///
/// @param type: UIPROGRESSBAR_LIMIT_OVER if the color is shown when the limit is exceeded,
/// UIPROGRESSBAR_LIMIT_BELOW if the color is shown when the limit is undercut
/// @param limit: Limiting value 0...1000
static inline void uv_uiprogressbar_set_limit(void *me, uiprogressbar_limit_e type,
		int16_t limit, color_t color) {
	this->limit_type = type;
	this->limit = limit;
	this->limit_color = color;
	uv_ui_refresh(this);
}


/// @brief: Set function is called by the parent window
void uv_uiprogressbar_step(void *me, uv_touch_st *touch, uint16_t step_ms);


#undef this

#endif

#endif /* UV_HAL_INC_UI_UV_UIPROGRESSBAR_H_ */
