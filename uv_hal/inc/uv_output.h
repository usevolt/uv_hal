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
#include "uv_pwm.h"
#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#if CONFIG_OUTPUT



#if !defined(CONFIG_OUTPUT_DITHER_FREQ)
#error "CONFIG_OUTPUT_DITHER_FREQ should define the dither frequency in Hz"
#endif


/// @brief: Defines the state of a single thruster power supply
enum {
	OUTPUT_STATE_OFF = 0,
	OUTPUT_STATE_ON,
	OUTPUT_STATE_OVERLOAD,
	OUTPUT_STATE_FAULT
};
typedef uint8_t uv_output_state_e;




/// @brief: Defines the output types
///
typedef enum {
	/// @brief: output is ON/OFF, current sense feedback not mandatory
	OUTPUT_MODE_DIGITAL = 0,
#if CONFIG_PWM
	/// @brief: output is PWM, current sense feedback not mandatory
	OUTPUT_MODE_PWM,
	/// @brief: output is proportional solenoid with dither, current sense feedback mandatory.
	/// Dither frequency and amplitude can be configured within each output_st but
	/// main PWM frequency has to be configured via CONFIG_PWM_* defines, as
	/// output_st module depends on uv_pwm.h to generate the PWM signal.
	OUTPUT_MODE_SOLENOID,
#endif
	OUTPUT_MODE_COUNT
} output_mode_e;



typedef struct {
	/// @brief: Output mode. see output_type_e for details.
	output_mode_e mode;
	/// @brief: Target value is mode dependent.
	/// OUTPUT_MODE_DIGITAL: NC,
	/// OUTPUT_MODE_PWM: PWM duty cycle,
	/// OUTPUT_MODE_SOLENOID: mA driven to the valve
	uint16_t target_val;
	/// @brief: ADC channel of the sense resistor
	uv_adc_channels_e adc_chn;
	/// @brief: Current sense resistor value in milliohms
	uint16_t sense_mohm;
	/// @brief: Current sense resistor amplification (defaults to 50)
	uint16_t sense_ampl;
	/// @brief: Current min limit in mA
	uint16_t limit_min;
	/// @brief: Current max limit in mA
	uint16_t limit_max;
	uv_moving_aver_st moving_avg;
	/// @brief: Holds the moving average output value
	uint16_t current;
	/// @brief: output module state
	uv_output_state_e state;
	union {
		/// @brief: gpio pin for the gate driving
		uv_gpios_e gate_io;
		/// @brief: PWM output for valve driving
		uint8_t pwm_channel;
	};
	struct {
		/// @brief: Delay for calculating the dither.
		///
		/// @note: Dither frequency has to be lower than step function cycle frequency!
		uv_delay_st delay;
		/// @brief: Dither addition value. This is added and subtracted
		/// periodically from the actual target value to implement dither
		int8_t addition;
	} dither;
	/// @brief: EMCY messages to be triggered in overcurrent / undercurrent situations
	uint32_t emcy_overload;
	uint32_t emcy_fault;
} uv_output_st;



/// @brief: Initializes the output driver module
///
/// @param mode: Mode of the output. See output_mode_e for more details.
/// @param adc_chn: ADC channel for current sensing. Set to 0 if current sensing is not implemented.
/// @param io_pwm: MOSFET gate IO or PWM channel, depending on which mode the output is on.
void uv_output_init(uv_output_st *this, const output_mode_e mode,
		const uv_adc_channels_e adc_chn, const uint32_t io_pwm,
		const uint16_t sense_mohm, const uint16_t min_val, const uint16_t max_val,
		const uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault);


/// @brief: Sets the current sense amplification value. Defaults to 50
static inline void uv_output_set_ampl(uv_output_st *this, const uint16_t value) {
	this->sense_ampl = value;
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
	return uv_moving_aver_get_val(&this->moving_avg);
}

/// @brief: Sets the output target value. The operation of this function depends on
/// the mode of the output. on OUTPUT_MODE_DIGITAL this is same as calling *uv_output_set_state*
/// with parameter *OUTPUT_STATE_ON* or *OUTPUT_STATE_OFF*. On *OUTPUT_MODE_PWM*
/// *value* is evaluated as PWM duty cycle, same way as when calling *uv_pwm_set*.
/// On *OUTPUT_MODE_SOLENOID* *value* indicates mA driven on the output.
void uv_output_set(uv_output_st *this, const uint16_t value);


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state);


/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms);





#endif


#endif /* UV_HAL_INC_UV_OUTPUT_H_ */
