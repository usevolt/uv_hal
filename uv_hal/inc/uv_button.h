/*
 * uv_button.h
 *
 *  Created on: Aug 11, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_BUTTON_H_
#define UV_HAL_INC_UV_BUTTON_H_


#include "uv_hal_config.h"

#if CONFIG_BUTTON
#include "uv_utilities.h"
#include "uv_gpio.h"
#include <stdbool.h>
#include <stdint.h>



#if !defined(CONFIG_BUTTON_LONG_PRESS_TIME_MS)
#error "CONFIG_BUTTON_LONG_PRESS_TIME_MS should define the time in milliseconds which it takes to trigger a \
long press event."
#endif
#if !defined(CONFIG_BUTTON_SEQUENTAL_PRESS_TIME_MS)
#error "CONFIG_BUTTON_SEQUENTAL_PRESS_TIME_MS should define the time in milliseconds which it takes to trigger\
 a sequental presses."
#endif
#if !defined(CONFIG_BUTTON_SEQ_PRESS_ACCELERATION)
#error "CONFIG_BUTTON_SEQ_PRESS_ACCELERATION should define the acceleration for sequental presses.\
 Set as 0 to disable acceleration."
#endif


/// @brief: Structure for buttons.
/// @note: Doesn't provide an actual button debouncing! Debouncing should be implemented
/// by adjusting the step time cycle. From the inverted nyquist theorem: Step cycle determines the
/// maximum detected smaple frequency. Usually 20 ms is good.
typedef struct {
	/// @brief: True only for one step cycle when the button was pressed
	bool pressed;
	/// @brief: True only for one step cycle when the button was released
	bool released;
	/// @brief: True when the button is held down
	bool is_down;
	bool invert;
	bool long_pressed;
	bool is_long_press;
	uv_gpios_e gpio;
	int delay;
	uint32_t press_count;
} uv_button_st;


/// @brief: Returns true if the button was pressed on this step cycle
static inline bool uv_button_pressed(uv_button_st *b) {
	return b->pressed;
}
/// @brief: Returns true if the button was released on this step cycle
static inline bool uv_button_released(uv_button_st *b) {
	return b->released;
}
/// @brief: Returns true if the button is down
static inline bool uv_button_is_down(uv_button_st *b) {
	return b->is_down;
}
/// @brief: Returns true if the button was long pressed this step cycle
static inline bool uv_button_long_pressed(uv_button_st *b) {
	return (b->long_pressed);
}

/// @brief: Initializes the button structure
uv_errors_e uv_button_init(uv_button_st *b, uv_gpios_e gpio, bool invert);

/// @brief: Should be called every step cycle
uv_errors_e uv_button_step(uv_button_st *b, unsigned int step_ms);

#endif

#endif /* UV_HAL_INC_UV_BUTTON_H_ */
