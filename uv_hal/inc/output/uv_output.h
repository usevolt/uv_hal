/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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
	// overload detected in the current sense
	OUTPUT_STATE_OVERLOAD,
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
	/// @brief: Current max limit in mA
	uint16_t limit_max;
	/// @brief: Fault limit in mA
	uint16_t limit_fault;
	uv_moving_aver_st moving_avg;
	/// @brief: Holds the moving average output value
	uint16_t current;
	/// @brief: output module state
	uv_output_state_e state;
	/// @brief: gpio pin for the gate driving
	uv_gpios_e gate_io;
	/// @brief: EMCY messages to be triggered in overcurrent / undercurrent situations
	uint32_t emcy_overload;
	uint32_t emcy_fault;
} uv_output_st;



/// @brief: Initializes the output driver module
///
/// @param mode: Mode of the output. See output_mode_e for more details.
/// @param adc_chn: ADC channel for current sensing. Set to 0 if current sensing is not implemented.
/// @param io_pwm: MOSFET gate IO or PWM channel, depending on which mode the output is on.
void uv_output_init(uv_output_st *this,  uv_adc_channels_e adc_chn, uv_gpios_e gate_io,
		uint16_t sense_ampl, uint16_t max_val, uint16_t fault_val,
		uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault);


/// @brief: Sets the current sense amplification value. Defaults to 50
static inline void uv_output_set_ampl(uv_output_st *this, const uint16_t value) {
	this->sense_ampl = value;
}

/// @brief: Sets the maximum allowed current value
static inline void uv_output_set_max(uv_output_st *this, uint16_t value) {
	this->limit_max = value;
}

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
