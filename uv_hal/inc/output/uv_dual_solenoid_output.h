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


#ifndef UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_
#define UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_




#include <uv_hal_config.h>
#include "uv_solenoid_output.h"


#if CONFIG_DUAL_SOLENOID_OUTPUT

#if !CONFIG_SOLENOID_OUTPUT
#error "CONFIG_SOLENOID_OUTPUT has to be enabled in order to use uv_dual_solenoid_output module."
#endif


typedef enum {
	DUAL_OUTPUT_SOLENOID_A = 0,
	DUAL_OUTPUT_SOLENOID_B,
	DUAL_OUTPUT_SOLENOID_COUNT
} uv_dual_solenoid_output_solenoids_e;


#define DUAL_SOLENOID_ACC_MAX	100
#define DUAL_SOLENOID_DEC_MAX	100

/// @brief: Configuration structure for the dual solenoid module
typedef struct {
	//NOTE: All of these variables should
	uv_solenoid_output_conf_st solenoid_conf[DUAL_OUTPUT_SOLENOID_COUNT];
	/// @brief: Control value acceleration factor, from 0 ... 100
	uint16_t acc;
	/// @brief: Control value deceleration factor, from 0 ... 100
	uint16_t dec;
	/// @brief: Inverts the solenoid direction
	uint16_t invert;
} uv_dual_solenoid_output_conf_st;



/// @brief: Dual solenoid output module. Works as a data structure for controlling dual
/// direction proportional valves.
typedef struct {
	// solenoids
	uv_solenoid_output_st solenoid[DUAL_OUTPUT_SOLENOID_COUNT];

	// parameter configurations
	uv_dual_solenoid_output_conf_st *conf;

	// the requested target output value from -1000 ... 1000, the actual drive current is
	// based on solenoid output configurations.
	int16_t target_req;
	// the actual target value. This is smoothened with acc and dec.
	int16_t target;
	uv_pid_st target_pid;

	// signed output current
	int16_t current_ma;
} uv_dual_solenoid_output_st;




/// @brief: Initializes the dual solenoid output module
void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_dual_solenoid_output_conf_st *conf,
		uv_pwm_channel_t pwm_a, uv_pwm_channel_t pwm_b,
		uv_adc_channels_e adc_common,
		uint16_t dither_freq, int16_t dither_ampl,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload_a, uint32_t emcy_overload_b,
		uint32_t emcy_fault_a, uint32_t emcy_fault_b);


/// @brief: Dual solenoid step function
void uv_dual_solenoid_output_step(uv_dual_solenoid_output_st *this, uint16_t step_ms);




/// @brief: Disables the dual solenoid output module
static inline void uv_dual_solenoid_output_disable(uv_dual_solenoid_output_st *this) {
	uv_solenoid_output_disable(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	uv_solenoid_output_disable(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
}


/// @brief: Enabled the dual solenoid output module
static inline void uv_dual_solenoid_output_enable(uv_dual_solenoid_output_st *this) {
	uv_solenoid_output_enable(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	uv_solenoid_output_enable(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
}


/// @brief: Returns the current measured from the sense feedback
static inline int16_t uv_dual_solenoid_output_get_current(uv_dual_solenoid_output_st *this) {
	return this->current_ma;
}

/// @brief: Sets the output current.
///
/// @param value: current in -1000 ... 1000 which is set to the actual solenoid output modules
static inline void uv_dual_solenoid_output_set(uv_dual_solenoid_output_st *this, int16_t value) {
	this->target_req = value;
}


/// @brief: Copies the configuration settings to the module
void uv_dual_solenoid_output_set_conf(uv_dual_solenoid_output_st *this,
					uv_dual_solenoid_output_conf_st *conf);
	

/// @brief: Returns the configuration parameter structure
static inline uv_dual_solenoid_output_conf_st *uv_dual_solenoid_output_get_conf(uv_dual_solenoid_output_st *this) {
	return this->conf;
}


/// @brief: returns the state of the solenoid output module
static inline uv_output_state_e uv_dual_solenoid_output_get_state(uv_dual_solenoid_output_st *this,
		uv_dual_solenoid_output_solenoids_e solenoid) {
	return uv_solenoid_output_get_state(&this->solenoid[solenoid]);
}


#endif

#endif /* UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_ */
