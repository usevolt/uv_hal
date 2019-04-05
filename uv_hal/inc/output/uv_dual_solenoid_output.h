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
#if !defined(CONFIG_DUAL_SOLENOID_ACC_DEF)
#error "CONFIG_DUAL_SOLENOID_ACC_DEF should define the default value for acceleration factor, 0 ... 100"
#endif
#if !defined(CONFIG_DUAL_SOLENOID_ACC_DEF)
#error "CONFIG_DUAL_SOLENOID_ACC_DEF should define the default value for acceleration factor, 0 ... 100"
#endif


typedef enum {
	DUAL_OUTPUT_SOLENOID_A = 0,
	DUAL_OUTPUT_SOLENOID_B,
	DUAL_OUTPUT_SOLENOID_COUNT
} uv_dual_solenoid_output_solenoids_e;



#define DUAL_SOLENOID_ACC_MAX	100
#define DUAL_SOLENOID_DEC_MAX	100
#define DUAL_SOLENOID_VALUE_MAX	1000
#define DUAL_SOLENOID_VALUE_MIN	-1000

#define DUAL_SOLENOID_OUTPUT_A_MIN_MA_SUBINDEX			SOLENOID_OUTPUT_MIN_MA_SUBINDEX
#define DUAL_SOLENOID_OUTPUT_A_MAX_MA_SUBINDEX			SOLENOID_OUTPUT_MAX_MA_SUBINDEX
#define DUAL_SOLENOID_OUTPUT_A_MIN_PERCENT_SUBINDEX		SOLENOID_OUTPUT_MIN_PERCENT_SUBINDEX
#define DUAL_SOLENOID_OUTPUT_A_MAX_PERCENT_SUBINDEX		SOLENOID_OUTPUT_MAX_PERCENT_SUBINDEX
#define DUAL_SOLENOID_OUTPUT_A_ONOFF_MODE_SUBINDEX		SOLENOID_OUTPUT_ONOFF_MODE_SUBINDEX
#define DUAL_SOLENOID_OUTPUT_B_MIN_MA_SUBINDEX			(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT + SOLENOID_OUTPUT_MIN_MA_SUBINDEX)
#define DUAL_SOLENOID_OUTPUT_B_MAX_MA_SUBINDEX			(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT + SOLENOID_OUTPUT_MAX_MA_SUBINDEX)
#define DUAL_SOLENOID_OUTPUT_B_MIN_PERCENT_SUBINDEX		(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT + SOLENOID_OUTPUT_MIN_PERCENT_SUBINDEX)
#define DUAL_SOLENOID_OUTPUT_B_MAX_PERCENT_SUBINDEX		(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT + SOLENOID_OUTPUT_MAX_PERCENT_SUBINDEX)
#define DUAL_SOLENOID_OUTPUT_B_ONOFF_MODE_SUBINDEX		(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT + SOLENOID_OUTPUT_ONOFF_MODE_SUBINDEX)
#define DUAL_SOLENOID_OUTPUT_ACC_SUBINDEX				(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT * 2 + 1)
#define DUAL_SOLENOID_OUTPUT_DEC_SUBINDEX				(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT * 2 + 2)
#define DUAL_SOLENOID_OUTPUT_INVERT_SUBINDEX			(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT * 2 + 3)
#define DUAL_SOLENOID_OUTPUT_ASSEMBLY_INVERT_SUBINDEX	(SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT * 2 + 4)

/// @brief: Configuration structure for the dual solenoid module
typedef struct {
	//NOTE: All of these variables should
	uv_solenoid_output_conf_st solenoid_conf[DUAL_OUTPUT_SOLENOID_COUNT];
	/// @brief: Control value acceleration factor, from 0 ... 100
	uint16_t acc;
	/// @brief: Control value deceleration factor, from 0 ... 100
	uint16_t dec;
	/// @brief: Inverts the solenoid direction. Note that this actually doesn't do
	/// anything here in dual_solenoid_output. Rather, it can be used in user application.
	uint16_t invert;
	/// @brief: Another invertion meant for service configurations. **invert** should be
	/// meant for user application settings, **assembly_invert** for service.
	uint16_t assembly_invert;
} uv_dual_solenoid_output_conf_st;


typedef struct {
	uv_solenoid_output_limitconf_st solenoid_limitconf[DUAL_OUTPUT_SOLENOID_COUNT];
} uv_dual_solenoid_output_limitconf_st;


/// @brief: Resets the dual solenoid output configuration module to default settings
void uv_dual_solenoid_output_conf_reset(uv_dual_solenoid_output_conf_st *this,
		uv_dual_solenoid_output_limitconf_st *limitconf);


/// @brief: Dual solenoid output module. Works as a data structure for controlling dual
/// direction proportional valves.
typedef struct {
	// solenoids
	uv_solenoid_output_st solenoid[DUAL_OUTPUT_SOLENOID_COUNT];

	// parameter configurations
	uv_dual_solenoid_output_conf_st *conf;
	uv_dual_solenoid_output_limitconf_st *limitconf;

	// the requested target output value from -1000 to 1000, the actual drive current is
	// based on solenoid output configurations.
	int16_t target_req;
	// the actual target value. This is smoothened with acc and dec.
	int16_t target;
	// Helper variable to hold more precise target value. With this
	// the PID calculations are done with greater precision than the output actually is
	int16_t target_mult;
	uv_pid_st target_pid;
	uv_delay_st target_delay;

	// signed output current
	int16_t current_ma;
	// signed output value (depends on the mode)
	int16_t out;
} uv_dual_solenoid_output_st;




/// @brief: Initializes the dual solenoid output module
void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_dual_solenoid_output_conf_st *conf,
		uv_dual_solenoid_output_limitconf_st *limitconf,
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


/// @brief: Returns the output value, which depends on the mode of the solenoid output.
/// If the mode is current or onoff, measured current is returned. If the mode is pwm,
/// the pwm ppt is returned.
static inline int16_t uv_dual_solenoid_output_get_out(uv_dual_solenoid_output_st *this) {
	return this->out;
}

/// @brief: Sets the output current.
///
/// @param value: current in -1000 ... 1000 which is set to the actual solenoid output modules
static inline void uv_dual_solenoid_output_set(uv_dual_solenoid_output_st *this, int16_t value) {
	this->target_req = value;
}

/// @brief: Returns the target value, in -1000 ... 1000
static inline int16_t uv_dual_solenoid_output_get_target(uv_dual_solenoid_output_st *this) {
	return this->target;
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


/// @brief: Sets the mode for both outputs. The mode can be either current oŕ pwm.
/// The B output is updated in the dual_solenoid_output_step function according to A output.
static inline void uv_dual_solenoid_output_set_mode(uv_dual_solenoid_output_st *this,
		uv_solenoid_output_mode_st value) {
	uv_solenoid_output_set_mode(&this->solenoid[0], value);
}


#endif

#endif /* UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_ */
