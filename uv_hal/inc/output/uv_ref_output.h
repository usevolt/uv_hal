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


#ifndef UV_HAL_INC_UV_REF_OUTPUT_H_
#define UV_HAL_INC_UV_REF_OUTPUT_H_


#include <uv_hal_config.h>
#include <uv_dual_solenoid_output.h>
#include "uv_utilities.h"
#include "uv_filters.h"
#include "uv_pid.h"
#include "uv_pwm.h"
#include "uv_adc.h"


#if CONFIG_REF_OUTPUT


#define REF_OUTPUT_TOGGLE_THRESHOLD_DEFAULT	500
#define REF_OUTPUT_LIMIT_MS_DEFAULT			0
#define REF_OUTPUT_ENABLE_DELAY_MS_DEFAULT	0


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




#define REF_OUTPUT_ACC_MAX						100
#define REF_OUTPUT_ACC_MIN						20
#define REF_OUTPUT_DEC_MAX						100
#define REF_OUTPUT_DEC_MIN						20
#define REF_OUTPUT_VALUE_MAX					1000
#define REF_OUTPUT_VALUE_MIN					-1000
#define REF_OUTPUT_VALUE_DEFAULT				0



/// @brief: Defines the output mode. See uv_dual_solenoid_output_st for the details.
typedef uv_dual_solenoid_output_modes_e uv_ref_output_mode_e;


/// @brief: Data structure for ref output configuration data.
/// This can be stored in non-volatile memory.
typedef uv_dual_solenoid_output_conf_st uv_ref_output_conf_st;


/// @brief: Limit configurations uses the same limitconf structure as uv_dual_solenoid_output.
/// The Dual solenoid positive side (array index 0) is the maximum limit and
/// the negative side (array index 1) is the minimum limit. both of these values should be given
/// as in dual solenoid output, i.e. the minimum value is given on 0 ... 1000, wich indicates
/// the negative value. As ref output has only one channel, this number is internally converted
/// to 0 ... 500 output scale.
typedef uv_dual_solenoid_output_limitconf_st uv_ref_output_limitconf_st;


/// @brief: Resets the output values to defaults
void uv_ref_output_conf_reset(uv_ref_output_conf_st *conf, uv_ref_output_limitconf_st *limitconf);



/// @brief: defines a single ref output lookup table entry. An array of these
/// is used to linearily interpolate the output pwm duty cycle
typedef struct {
	// The PWM duty cycle which results in a *rel_value*
	uint16_t pwm_value;
	// The relative value as 1000 * output_value / vdd. Should be 0 ... 1000
	uint16_t rel_value;
} uv_ref_output_lookup_st;


typedef struct {
	EXTENDS(uv_output_st);

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
	/// @brief: The request for the target value. This is set via the user application
	int16_t target_req;
	int16_t last_target_req;
	/// @brief: Stores the current PWM duty cycle
	uint16_t pwm;
	/// @brief: Stores the output value on every specific moment. The value is in mv
	int16_t out;
	/// @brief: PWM channel configured for this output
	uv_pwm_channel_t pwm_chn;

	const uv_ref_output_lookup_st *lookuptable;
	uint8_t lookuptable_len;

	uv_hysteresis_st toggle_hyst;
	uint8_t last_hyst;
	// tells the output state on ONOFFTOGGLE modes. 0, -1 or 1 depending on
	// the direction of the toggle.
	int8_t toggle_on;
	uv_delay_st toggle_delay;
	uint32_t toggle_limit_ms;
	uint32_t enable_delay_ms;
	uv_delay_st enable_delay;


	// the mode of this output
	uv_ref_output_mode_e mode;

} uv_ref_output_st;



/// @brief: Initializes the ref output
///
/// @param adc_chn: Current sense feedback analog channel
/// @param pwm_chn: The PWM channel used to driving the output
/// @param sense_ampl: Amplification for current sense feedback. ADC value
/// from current sense feedback is multiplied with this in order to get milliamps.
/// @param fault_ma: The limit in ma which is indicated being a fault. Ref output
/// should not be used as a power output, but this is used just to protect
/// the vnd5050 power mosfet in case of shortcircuit.
/// @param lookup_table: Pointer to a const array of uv_ref_output_lookup_st entries.
/// The table should be in rising order, i.e. the first element should have the
/// smallest relative value and the last one the biggest.
/// @param lookup_table_len: The length of the lookup table in element count
void uv_ref_output_init(uv_ref_output_st *this,
		uv_ref_output_conf_st *conf_ptr, uv_ref_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uv_adc_channels_e adc_chn, uint16_t sense_ampl,
		uint16_t fault_ma, uint32_t emcy_fault,
		const uv_ref_output_lookup_st *lookup_table, uint8_t lookup_table_len);


/// @brief: Sets the output mode
static inline void uv_ref_output_set_mode(uv_ref_output_st *this, uv_ref_output_mode_e value) {
	this->mode = value;
}


/// @brief: Gets the ref output mode
static inline uv_ref_output_mode_e uv_ref_output_get_mode(uv_ref_output_st *this) {
	return this->mode;
}


static inline uv_output_state_e uv_ref_output_get_state(uv_ref_output_st *this) {
	return uv_output_get_state((uv_output_st*) this);
}


/// @brief: Sets the toggle threshold value which has to be exceeded in
/// either direction to trigger the toggle state. 0 .. 1000.
static inline void uv_ref_output_set_onofftoggle_threshold(uv_ref_output_st *this, uint16_t value) {
	this->toggle_hyst.trigger_value = value;
}


/// @brief: Sets the toggle limit in milliseconds. When the output has been ON after this time,
/// The toggle state is cleared.
static inline void uv_ref_output_set_onofftoggle_limit_ms(uv_ref_output_st *this, uint32_t value) {
	this->toggle_limit_ms = value;
}

/// @brief: Sets the enable delay time in milliseconds. The output
/// is set ON only after the request has been active for this delay.
static inline void uv_ref_output_set_enable_delay_ms(
		uv_ref_output_st *this, uint32_t value) {
	this->enable_delay_ms = value;
}


/// @brief: Step funtion
void uv_ref_output_step(uv_ref_output_st *this, uint16_t step_ms);


/// @brief: Sets the ref output target value in ppt.
/// in PWM mode the duty cycle.
/// Values should be -1000 ... 1000.
void uv_ref_output_set(uv_ref_output_st *this, int16_t value);


/// @brief: Returns the output value. The value is -1000 ... 1000
static inline int16_t uv_ref_output_get_out(uv_ref_output_st *this) {
	return this->out;
}


static inline int16_t uv_ref_output_get_target(uv_ref_output_st *this) {
	return this->target;
}


/// @brief: Disables the output. Output can be enabled only by calling
/// *uv_ref_output_enable*.
static inline void uv_ref_output_disable(uv_ref_output_st *this) {
	uv_output_disable((uv_output_st*) this);
}


/// @brief: Enabled the output once it's disabled with *uv_ref_output_disable*.
static inline void uv_ref_output_enable(uv_ref_output_st *this) {
	uv_output_enable((uv_output_st*) this);
}


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
