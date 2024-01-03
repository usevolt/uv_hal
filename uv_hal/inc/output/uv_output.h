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

#ifndef UV_HAL_INC_UV_OUTPUT_H_
#define UV_HAL_INC_UV_OUTPUT_H_



// @file: Power output module

#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_filters.h"
#include "uv_adc.h"
#include "uv_gpio.h"
#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#if CONFIG_OUTPUT





/// @brief: Defines the state of a single thruster power supply
enum {
	// output is kept at 0
	OUTPUT_STATE_OFF = 0,
	// output is on
	OUTPUT_STATE_ON,
	// openloop detected in the current sense
	OUTPUT_STATE_OPENLOOP,
	// fault value detected in the current sense
	OUTPUT_STATE_FAULT,
	// output is disabled and can be set to ON only after enabling it
	OUTPUT_STATE_DISABLED
};
typedef uint8_t uv_output_state_e;




/// @brief: Basic output module. This can be inherited to implement various
/// other output modules.
typedef struct {
	/// @brief: ADC channel of the sense resistor
	uv_adc_channels_e adc_chn;
	/// @brief: Current sense resistor amplification. Multiplying the
	/// ADC reading with this value should result into microamperes
	/// which is then converted into milliamps in the output_st.
	uint16_t sense_ampl;
	/// @brief: If the output uses on/off stat feedback,
	/// *stat_io* should define the GPIO pin used for the feedback.
	/// Low state in *stat_io* is read as a fault condition. in this case,
	/// adc_chn, sense_ampl, limit values and overload emcy are not used.
	/// *stat_io* defaults to 0, i.e. disabled. Enable it with a call to
	/// *uv_output_set_stat_io*
	uv_gpios_e stat_io;
	/// @brief: Current max limit in mA
	uint16_t limit_max_ma;
	uint16_t limit_fault_ma;
	uv_moving_aver_st moving_avg;
	/// @brief: Holds the moving average output value in milliamps
	uint16_t current;
	/// @brief: output module state
	uv_output_state_e state;
	/// @brief: gpio pin for the gate driving
	uv_gpios_e gate_io;
	// Set to true if the gate IO logic should be inverted
	bool gate_io_invert;

	// delay for fault freexing
	uv_delay_st fault_freeze_delay;

	/// brief: Current calculation function pointer. This function should calculate the current
	/// in milliamps from the adc reading
	uint16_t (*current_func)(void *this_ptr, uint16_t adc);

	/// @brief: EMCY messages to be triggered in overcurrent / undercurrent situations
	uint32_t emcy_openloop;
	uint32_t emcy_fault;
} uv_output_st;



/// @brief: Initializes the output driver module
///
/// @param mode: Mode of the output. See output_mode_e for more details.
/// @param adc_chn: ADC channel for current sensing. Set to 0 if current sensing is not implemented.
/// @param io_pwm: MOSFET gate IO or PWM channel, depending on which mode the output is on.
void uv_output_init(uv_output_st *this,  uv_adc_channels_e adc_chn, uv_gpios_e gate_io,
		uint16_t sense_ampl, uint16_t max_val_ma, uint16_t fault_val_ma,
		uint16_t moving_avg_count, uint32_t emcy_openloop, uint32_t emcy_fault);


/// @brief: Sets the current calculation function
static inline void uv_output_set_current_func(uv_output_st *this,
		uint16_t (*current_func)(void *this_ptr, uint16_t adc)) {
	this->current_func = current_func;
}


/// @brief: Freezes the fault detection for *ms* given time. This can be used
/// to prevent unintentional faults.
///
/// @note: More than 100 ms freeze is not suggested, as this might burn the whole controller
/// in case of short circuit. The VN5T100 amplifier won't sustain too long short circuit
static inline void uv_output_freeze_fault_detection(uv_output_st *this, uint32_t ms) {
	uv_delay_init(&this->fault_freeze_delay, ms);
}


/// @brief: Sets the current sense amplification value. Defaults to 50
static inline void uv_output_set_ampl(uv_output_st *this, const uint16_t value) {
	this->sense_ampl = value;
}

/// @brief: Sets the maximum allowed current value
static inline void uv_output_set_max(uv_output_st *this, uint16_t value) {
	this->limit_max_ma = value;
}



/// @brief: Sets the output to work in a stat io feedback mode. See uv_output_st member
/// variable documentation for more info.
void uv_output_set_stat_feedback_io(uv_output_st* this, uv_gpios_e gpio);

/// @brief: Returns the state of the output
static inline uv_output_state_e uv_output_get_state(const uv_output_st *this) {
	return this->state;
}

/// @brief: Sets the sense amplification. Defaults to 50.
static inline void uv_output_set_sense_ampl(uv_output_st *this, uint16_t sense_ampl) {
	this->sense_ampl = sense_ampl;
}

/// @brief: Returns the current measured form the current sense feedback
static inline uint16_t uv_output_get_current(uv_output_st *this) {
	return this->current;
}


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state);
static inline void uv_output_set(uv_output_st *this, uv_output_state_e state) {
	uv_output_set_state(this, state);
}


static inline void uv_output_set_gate_io_invert(uv_output_st *this, bool value) {
	this->gate_io_invert = value;
}

static inline bool uv_output_get_gate_io_invert(uv_output_st *this) {
	return this->gate_io_invert;
}

/// @brief: Enabled output. Needs to be called only after calling *uv_output_disable* in order
/// to enable it once again. Once enabled, output will be in state OFF.
void uv_output_enable(uv_output_st *this);

/// @brief: Disables the output. Output can be enabled only by calling *uv_output_enable*
static inline void uv_output_disable(uv_output_st *this) {
	this->state = OUTPUT_STATE_DISABLED;
}

/// @brief: Sets the output enablation according to **value**
static inline void uv_output_set_enabled(uv_output_st *this, bool value) {
	value ? uv_output_enable(this) : uv_output_disable(this);
}

/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms);


#endif


#endif /* UV_HAL_INC_UV_OUTPUT_H_ */
