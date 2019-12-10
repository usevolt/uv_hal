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
#include "uv_prop_output.h"
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
#if !CONFIG_PROP_OUTPUT
#error "uv_ref_output requires PROP_OUTPUT to be enabled."
#endif






/// @brief: Resets the output values to defaults
static inline void uv_ref_output_conf_reset(uv_prop_output_conf_st *conf,
		uv_prop_output_limitconf_st *limitconf) {
	uv_prop_output_conf_reset(conf, limitconf);
}



/// @brief: defines a single ref output lookup table entry. An array of these
/// is used to linearily interpolate the output pwm duty cycle
typedef struct {
	// The PWM duty cycle which results in a *rel_value*
	uint16_t pwm_value;
	// The relative value as 1000 * output_value / vdd. Should be 0 ... 1000
	uint16_t rel_value;
} uv_ref_output_lookup_st;


typedef struct {
	EXTENDS(uv_prop_output_st);

	/// @brief: Stores the output value on every specific moment. The value is in ppt
	int16_t out;
	/// @brief: Stores the current PWM duty cycle
	uint16_t pwm;
	/// @brief: PWM channel configured for this output
	uv_pwm_channel_t pwm_chn;

	const uv_ref_output_lookup_st *lookuptable;
	uint8_t lookuptable_len;

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
		uv_prop_output_conf_st *conf, uv_prop_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uv_adc_channels_e adc_chn, uint16_t sense_ampl,
		uint16_t fault_ma, uint32_t emcy_fault,
		const uv_ref_output_lookup_st *lookup_table, uint8_t lookup_table_len);


/// @brief: Sets the output mode
static inline void uv_ref_output_set_mode(uv_ref_output_st *this,
		uv_prop_output_modes_e value) {
	uv_prop_output_set_mode((uv_prop_output_st*) this, value);
}


/// @brief: Gets the ref output mode
static inline uv_prop_output_modes_e uv_ref_output_get_mode(uv_ref_output_st *this) {
	return uv_prop_output_get_mode((uv_prop_output_st*) this);
}


static inline uv_output_state_e uv_ref_output_get_state(uv_ref_output_st *this) {
	return uv_output_get_state((uv_output_st*) this);
}


/// @brief: Sets the toggle threshold value which has to be exceeded in
/// either direction to trigger the toggle state. 0 .. 1000.
static inline void uv_ref_output_set_toggle_threshold(uv_ref_output_st *this,
		uint16_t value) {
	uv_prop_output_set_toggle_threshold((uv_prop_output_st*) this, value);
}


/// @brief: Sets the toggle limit in milliseconds. When the output has been ON after this time,
/// The toggle state is cleared.
static inline void uv_ref_output_set_toggle_limit_ms(uv_ref_output_st *this, uint32_t value) {
	uv_prop_output_set_toggle_limit_ms((uv_prop_output_st*) this, value);
}

/// @brief: Sets the enable delay time in milliseconds. The output
/// is set ON only after the request has been active for this delay.
static inline void uv_ref_output_set_enable_delay_ms(
		uv_ref_output_st *this, uint32_t value) {
	uv_prop_output_set_enable_delay_ms((uv_prop_output_st*) this, value);
}


/// @brief: Step funtion
void uv_ref_output_step(uv_ref_output_st *this, uint16_t step_ms);


/// @brief: Sets the ref output target value in ppt.
/// in PWM mode the duty cycle.
/// Values should be -1000 ... 1000.
static inline void uv_ref_output_set(uv_ref_output_st *this, int16_t value) {
	uv_prop_output_set((uv_prop_output_st*) this, value);
}


/// @brief: Returns the output value. The value is -1000 ... 1000
static inline int16_t uv_ref_output_get_out(uv_ref_output_st *this) {
	return this->out;
}


static inline int16_t uv_ref_output_get_target(uv_ref_output_st *this) {
	return uv_prop_output_get_target((uv_prop_output_st*) this);
}


/// @brief: Returns the direction of ONOFFTOGGLE mode state. -1 if negative direction is active,
/// 1 if positive is active, 0 otherwise.
static inline uint8_t uv_ref_output_get_onofftoggle_dir(uv_ref_output_st *this) {
	return uv_prop_output_get_onofftoggle_dir((uv_prop_output_st*) this);
}



/// @brief: Disables the output. Output can be enabled only by calling
/// *uv_ref_output_enable*.
static inline void uv_ref_output_disable(uv_ref_output_st *this) {
	uv_prop_output_disable((uv_prop_output_st*) this);
}


/// @brief: Enabled the output once it's disabled with *uv_ref_output_disable*.
static inline void uv_ref_output_enable(uv_ref_output_st *this) {
	uv_prop_output_enable((uv_prop_output_st*) this);
}


/// @brief: Copies the configuration parameters to the output
static inline void uv_ref_output_set_conf(uv_ref_output_st *this,
		uv_prop_output_conf_st *conf) {
	uv_prop_output_set_conf((uv_prop_output_st *) this, conf);
}


/// @brief: returns the configuration parameter structure
static inline uv_prop_output_conf_st *uv_ref_output_get_conf(uv_ref_output_st *this) {
	return uv_prop_output_get_conf((uv_prop_output_st*) this);
}


/// @brief: Returns the solenoid's current PWM duty cycle value
static inline uint16_t uv_ref_output_get_pwm_dc(uv_ref_output_st *this) {
	return this->pwm;
}


#endif


#endif /* UV_HAL_INC_UV_REF_OUTPUT_H_ */
