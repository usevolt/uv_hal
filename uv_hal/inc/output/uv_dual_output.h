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


#ifndef UV_HAL_INC_OUTPUT_UV_DUAL_OUTPUT_H_
#define UV_HAL_INC_OUTPUT_UV_DUAL_OUTPUT_H_


#include "uv_utilities.h"
#include "uv_output.h"
#include <uv_hal_config.h>


#if CONFIG_DUAL_OUTPUT
#if !CONFIG_OUTPUT
#error "Dual output depends on output module which should be enabled with CONFIG_OUTPUT set as 1."
#endif


/// @brief: Defines the output direction
typedef enum {
	/// Output is off
	DUAL_OUTPUT_OFF = 0,
	// Output is in positive direction (direction A)
	DUAL_OUTPUT_POS = 1,
	// Output is in negative direction (direction B)
	DUAL_OUTPUT_NEG = -1
} uv_dual_output_dir_e;

typedef struct {
	EXTENDS(uv_output_st);

	uv_dual_output_dir_e dir;
	uv_gpios_e gpio_pos;
	uv_gpios_e gpio_neg;
	bool invert;
	int32_t current;

} uv_dual_output_st;


/// @brief: Initializes the dual output
///
/// @param gpio_pos: GPIO pin used to drive the output in positive direction
/// @param gpio_neg: GPIO pin used to drive the output in negative direction
/// @param adc_chn: Current sense feedback analog channel
/// @param sense_ampl: Amplification for current sense feedback. ADC value
/// from current sense feedback is multiplied with this in order to get milliamps.
/// @param max_current: Maximum allowed current, which is used to detect overload situations.
/// @param fault_current: Current limit of which greater current is indicated as fault situation.
/// Should always be greater than **max_current**.
/// @param moving_avg_count: Count for current sense moving average filter.
/// @emcy_overload: CANopen EMCY message for overload situation
// @emcy_fault: CANopen EMCY message for fault situation
void uv_dual_output_init(uv_dual_output_st *this, uv_gpios_e gpio_pos, uv_gpios_e gpio_neg,
		uv_adc_channels_e adc_chn, uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault);


/// @brief: Step funtion
void uv_dual_output_step(uv_dual_output_st *this, uint16_t step_ms);


/// @brief: Sets the output's direction. If set to DUAL_OUTPUT_OFF, output switches itself to
/// OUTPUT_STATE_OFF state.
void uv_dual_output_set_dir(uv_dual_output_st *this, uv_dual_output_dir_e value);
static inline void uv_dual_output_set(uv_dual_output_st *this, uv_dual_output_dir_e value) {
	uv_dual_output_set_dir(this, value);
}

/// @brief: Returns the output direction
static inline uv_dual_output_dir_e uv_dual_output_get_dir(uv_dual_output_st *this) {
	return this->dir;
}


/// @brief: sets the direction invertion
static inline void uv_dual_output_set_invert(uv_dual_output_st *this, bool value) {
	this->invert = value;
}

/// @brief: Gets the direction invertion
static inline bool uv_dual_output_get_invert(uv_dual_output_st *this) {
	return this->invert;
}

/// @brief: Disables the output. Output can be enabled only by calling
/// *uv_dual_output_enable*.
static inline void uv_dual_output_disable(uv_dual_output_st *this) {
	uv_output_disable((uv_output_st *) this);
}

/// @brief: Enabled the output once it's disabled with *uv_dual_output_disable*.
static inline void uv_dual_output_enable(uv_dual_output_st *this) {
	uv_output_enable((uv_output_st *) this);
}

/// @brief: Sets the output state
static inline void uv_dual_output_set_state(uv_dual_output_st *this,
		uv_output_state_e state) {
	uv_output_set_state((uv_output_st *) this, state);
}

/// @brief: returns the state of the output
static inline uv_output_state_e uv_dual_output_get_state(
		const uv_dual_output_st *this) {
	return uv_output_get_state(((uv_output_st*) this));
}

/// @brief: Returns the current measured from the sense feedback
static inline int32_t uv_dual_output_get_current(uv_dual_output_st *this) {
	return this->current;
}



#endif

#endif /* UV_HAL_INC_OUTPUT_UV_DUAL_OUTPUT_H_ */
