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


#ifndef UV_HAL_INC_UV_REF_OUTPUT_H_
#define UV_HAL_INC_UV_REF_OUTPUT_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_filters.h"
#include "uv_pid.h"
#include "uv_pwm.h"
#include "uv_adc.h"


#if CONFIG_REF_OUTPUT


/// @file: defines a ref output module. Ref output works as a voltage reference. It
/// drives the output with PWM and measures the voltage back with a adc input.

#if !CONFIG_PID
#error "uv_ref_output requires uv_pid_st to be enabled with CONFIG_PID defined as 1."
#endif
#if !CONFIG_PWM
#error "uv_ref_output requires CONFIG_PWM to be enabled."
#endif


#if !defined(CONFIG_REF_POSMIN_PPT_DEF)
#error "CONFIG_REF_POSMIN_PPT_DEF should define the positive min ppt default value"
#endif
#if !defined(CONFIG_REF_POSMAX_PPT_DEF)
#error "CONFIG_REF_POSMAX_PPT_DEF should define the positive max ppt default value"
#endif
#if !defined(CONFIG_REF_NEGMAX_PPT_DEF)
#error "CONFIG_REF_NEGMAX_PPT_DEF should define the negative max ppt default value"
#endif
#if !defined(CONFIG_REF_NEGMAX_PPT_DEF)
#error "CONFIG_REF_NEGMAX_PPT_DEF should define the negative max ppt default value"
#endif
#if !defined(CONFIG_REF_ACC_DEF)
#error "CONFIG_REF_ACC_DEF should define the default acceleration factor for ref output"
#endif
#if !defined(CONFIG_REF_DEC_DEF)
#error "CONFIG_REF_DEC_DEF should define the default deceleration factor for ref output"
#endif
#if !defined(CONFIG_REF_LIMITMAX_PPT_DEF)
#error "CONFIG_REF_LIMITMAX_PPT_DEF should define the default maximum limit in ppt for ref output in relative mode"
#endif
#if !defined(CONFIG_REF_LIMITMIN_PPT_DEF)
#error "CONFIG_REF_LIMITMIN_PPT_DEF should define the default minimum limit in ppt for ref output in relative mode"
#endif
#if !defined(CONFIG_REF_LIMITMAX_MV_DEF)
#error "CONFIG_REF_LIMITMAX_MV_DEF should define the default maximum limit in mv for ref output in absolute mode"
#endif
#if !defined(CONFIG_REF_LIMITMIN_MV_DEF)
#error "CONFIG_REF_LIMITMIN_MV_DEF should define the default minimum limit in ppt for ref output in absolute mode"
#endif
#if !defined(CONFIG_REF_PID_P_DEF)
#error "CONFIG_REF_PID_P_DEF should define the default P factor for the absolute mode PID controller"
#endif
#if !defined(CONFIG_REF_PID_I_DEF)
#error "CONFIG_REF_PID_I_DEF should define the default I factor for the absolute mode PID controller"
#endif



#define REF_OUTPUT_POSMIN_PPT_SUBINDEX			1
#define REF_OUTPUT_POSMAX_PPT_SUBINDEX			2
#define REF_OUTPUT_NEGMIN_PPT_SUBINDEX			3
#define REF_OUTPUT_NEGMAX_PPT_SUBINDEX			4
#define REF_OUTPUT_SUBINDEX_COUNT				4


#define REF_OUTPUT_ACC_MAX						100
#define REF_OUTPUT_ACC_MIN						20
#define REF_OUTPUT_DEC_MAX						100
#define REF_OUTPUT_DEC_MIN						20
#define REF_OUTPUT_VALUE_MAX					1000
#define REF_OUTPUT_VALUE_MIN					-1000
#define REF_OUTPUT_VALUE_DEFAULT				0



typedef enum {
	// output voltage is relative to the supply voltage
	REF_OUTPUT_MODE_REL = 0,
	// output voltage is measured in absolute millivolts
	REF_OUTPUT_MODE_ABS,
	// output voltage is controlled digitally from 0 to vdd
	REF_OUTPUT_MODE_ONOFFREL,
	// output voltage is controlled digitally from 0 to limit mv
	REF_OUTPUT_MODE_ONOFFABS
} uv_ref_output_mode_e;


typedef enum {
	REF_OUTPUT_STATE_ENABLED = 0,
	REF_OUTPUT_STATE_DISABLED
} uv_ref_output_state_e;


/// @brief: Data structure for ref output configuration data.
/// This can be stored in non-volatile memory.
typedef struct {
	// positive minimum value ppt
	uint16_t posmin_ppt;
	// positive maximum PWM ppt
	uint16_t posmax_ppt;
	// negative minimum PWM ppt
	uint16_t negmin_ppt;
	// negative maximum PWM ppt
	uint16_t negmax_ppt;
	// The acceleration factor
	uint16_t acc;
	// The deceleration factor
	uint16_t dec;
	// This invert is not used in this library. It is meant for user application.
	uint16_t invert;
	// Inverts the direction of output
	uint16_t assembly_invert;
} uv_ref_output_conf_st;


typedef struct {
	// The maximum limit for the output in ppt for mode REF_OUTPUT_MODE_REL.
	uint16_t limit_max_ppt;
	// The minimum limit for the output in ppt for mode REF_OUTPUT_MODE_REL.
	uint16_t limit_min_ppt;
	// The maximum limit for the output in mv for mode REF_OUTPUT_MODE_ABS.
	uint16_t limit_max_mv;
	// The minimum limit for the output in mv for mode REF_OUTPUT_MODE_ABS.
	uint16_t limit_min_mv;
} uv_ref_output_limitconf_st;


/// @brief: Resets the output values to defaults
void uv_ref_output_conf_reset(uv_ref_output_conf_st *conf, uv_ref_output_limitconf_st *limitconf);



typedef struct {
	// configuration parameters
	uv_ref_output_conf_st *conf;
	uv_ref_output_limitconf_st *limitconf;

	/// @brief: PID controller for controlling the output value in absolute mode.
	/// Should be as fast as possible
	uv_pid_st mv_pid;

	int16_t target_mult;
	uv_pid_st target_pid;
	uv_delay_st target_delay;
	/// @brief: Target value from -1000 ... 1000. This is scaled to the output value
	/// depending on the output mode.
	int16_t target;
	/// @brief: Stores the target value in millivolts
	int16_t target_mv;
	/// @brief: The request for the target value. This is set via the user application
	int16_t target_req;
	/// @brief: Stores the current PWM duty cycle
	uint16_t pwm;
	/// @brief: Stores the output value on every specific moment. The value is in mv
	uint16_t out;
	uv_moving_aver_st out_avg;
	/// @brief: PWM channel configured for this output
	uv_pwm_channel_t pwm_chn;
	uv_adc_channels_e adc_chn;
	uint16_t adc_mult;

	// the mode of this output
	uv_ref_output_mode_e mode;
	uv_ref_output_state_e state;

} uv_ref_output_st;




/// @brief: Initializes the ref output
///
/// @param adc_chn: Current sense feedback analog channel
/// @param pwm_chn: The PWM channel used to driving the output
/// @param sense_ampl: Amplification for current sense feedback. ADC value
/// from current sense feedback is multiplied with this in order to get milliamps.
/// @emcy_overload: CANopen EMCY message for overload situation
// @emcy_fault: CANopen EMCY message for fault situation
void uv_ref_output_init(uv_ref_output_st *this,
		uv_ref_output_conf_st *conf_ptr, uv_ref_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uv_adc_channels_e adc_chn, uint16_t sense_ampl);


/// @brief: Sets the output mode
static inline void uv_ref_output_set_mode(uv_ref_output_st *this, uv_ref_output_mode_e value) {
	this->mode = value;
}

/// @brief: Gets the ref output mode
static inline uv_ref_output_mode_e uv_ref_output_get_mode(uv_ref_output_st *this) {
	return this->mode;
}

static inline uv_ref_output_state_e uv_ref_output_get_state(uv_ref_output_st *this) {
	return this->state;
}

/// @brief: Step funtion
///
/// @param vdd_mv: The system supply voltage in millivolts. This has to be measured
/// for relative mode, since the output voltage is relative to the system voltage
void uv_ref_output_step(uv_ref_output_st *this, uint16_t vdd_mv, uint16_t step_ms);


/// @brief: Sets the ref output target value in ppt.
/// in PWM mode the duty cycle.
/// Values should be -1000 ... 1000.
void uv_ref_output_set(uv_ref_output_st *this, int16_t value);


/// @brief: Returns the output value. The value is -1000 ... 1000
static inline uint16_t uv_ref_output_get_out(uv_ref_output_st *this) {
	return this->out;
}

static inline int16_t uv_ref_output_get_target(uv_ref_output_st *this) {
	return this->target;
}

/// @brief: Disables the output. Output can be enabled only by calling
/// *uv_ref_output_enable*.
void uv_ref_output_disable(uv_ref_output_st *this);


/// @brief: Enabled the output once it's disabled with *uv_ref_output_disable*.
void uv_ref_output_enable(uv_ref_output_st *this);



/// @brief: Copies the configuration parameters to the output
static inline void uv_ref_output_set_conf(uv_ref_output_st *this,
		uv_ref_output_conf_st *conf) {
	this->conf = conf;
}


/// @brief: returns the configuration parameter structure
static inline uv_ref_output_conf_st *uv_ref_output_get_conf(uv_ref_output_st *this) {
	return this->conf;
}


/// @brief: Returns the solenoid's current PWM duty cycle value
static inline uint16_t uv_ref_output_get_pwm_dc(uv_ref_output_st *this) {
	return this->pwm;
}

#endif

#endif /* UV_HAL_INC_UV_REF_OUTPUT_H_ */
