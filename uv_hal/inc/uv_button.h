/*
 * uv_button.h
 *
 *  Created on: Aug 11, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_BUTTON_H_
#define UV_HAL_INC_UV_BUTTON_H_


#include "uv_hal_config.h"
#include "uv_utilities.h"
#include "uv_gpio.h"
#include <stdbool.h>
#include <stdint.h>


/// @brief: The time which the button needs to be pressed to trigger the long press event
#define BUTTON_LONG_PRESS_TIME_MS			1000


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


#endif /* UV_HAL_INC_UV_BUTTON_H_ */
