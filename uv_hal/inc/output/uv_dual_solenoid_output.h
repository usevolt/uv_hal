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


#ifndef UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_
#define UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_




#include <uv_hal_config.h>
#include "uv_solenoid_output.h"
#include "uv_prop_output.h"


#if CONFIG_DUAL_SOLENOID_OUTPUT

#if !CONFIG_SOLENOID_OUTPUT
#error "CONFIG_SOLENOID_OUTPUT has to be enabled in order to use uv_dual_solenoid_output module."
#endif


typedef enum {
	DUAL_OUTPUT_SOLENOID_A = 0,
	DUAL_OUTPUT_SOLENOID_B,
	DUAL_OUTPUT_SOLENOID_COUNT
} uv_dual_solenoid_output_solenoids_e;


/// @brief: Dual solenoid output module. Works as a data structure for controlling dual
/// direction proportional valves.
typedef struct {
	EXTENDS(uv_prop_output_st);

	// solenoids
	uv_solenoid_output_st solenoid[DUAL_OUTPUT_SOLENOID_COUNT];


	// the type of the solenoid outputs. Should be either current (default) or PWM
	uv_solenoid_output_mode_e out_type;
	// if true, both solenoids are controlled with the same control value.
	// if false, other solenoid is always at 0 duty cycle depending on the direction.
	bool unidir;
	// signed output current
	int16_t current_ma;
	// signed output value (depends on the mode)
	int16_t out;
	// delay for keeping pvg control signal for a bit longer than powering it
	uv_delay_st pvg_delay;
} uv_dual_solenoid_output_st;




/// @brief: Initializes the dual solenoid output module
void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_prop_output_conf_st *conf,
		uv_prop_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_a, uv_pwm_channel_t pwm_b,
		uv_adc_channels_e adc_common,
		uint16_t dither_freq, int16_t dither_ampl,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_openloop_a, uint32_t emcy_openloop_b,
		uint32_t emcy_fault_a, uint32_t emcy_fault_b);



/// @brief: Dual solenoid step function
void uv_dual_solenoid_output_step(uv_dual_solenoid_output_st *this, uint16_t step_ms);



static inline void uv_dual_solenoid_output_clear(uv_dual_solenoid_output_st *this) {
	uv_prop_output_clear((uv_prop_output_st*) this);
}



/// @brief: Sets the output type. Either current of PWM, defaults for current.
static inline void uv_dual_solenoid_output_set_type(uv_dual_solenoid_output_st *this,
		uv_solenoid_output_mode_e type) {
	this->out_type = type;
}



/// @brief: Getter for the output type
static inline uv_solenoid_output_mode_e uv_dual_solenoid_output_get_type(
		uv_dual_solenoid_output_st* this) {
	return this->out_type;
}



/// @brief: Sets the dual solenoid output to unidir mode, where both outputs
/// are controlled with the same value. Due to the limitations in the hardware,
/// the current measurement is calculated as the sum of currents.
void uv_dual_solenoid_output_set_unidir(uv_dual_solenoid_output_st *this,
		bool value);


/// @brief: Disables the dual solenoid output module
static inline void uv_dual_solenoid_output_disable(uv_dual_solenoid_output_st *this) {
	uv_prop_output_disable((uv_prop_output_st*) this);
	uv_solenoid_output_disable(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	uv_solenoid_output_disable(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
}



/// @brief: Enabled the dual solenoid output module
static inline void uv_dual_solenoid_output_enable(uv_dual_solenoid_output_st *this) {
	uv_prop_output_enable((uv_prop_output_st *) this);
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
	uv_prop_output_set((uv_prop_output_st *) this, value);
}



/// @brief: Returns the target value, in -1000 ... 1000
static inline int16_t uv_dual_solenoid_output_get_target(uv_dual_solenoid_output_st *this) {
	return uv_prop_output_get_target((uv_prop_output_st *) this);
}



/// @brief: Copies the configuration settings to the module
void uv_dual_solenoid_output_set_conf(uv_dual_solenoid_output_st *this,
					uv_prop_output_conf_st *conf);
	


/// @brief: Returns the configuration parameter structure
static inline uv_prop_output_conf_st *uv_dual_solenoid_output_get_conf(
		uv_dual_solenoid_output_st *this) {
	return uv_prop_output_get_conf((uv_prop_output_st *) this);
}



/// @brief: returns the state of the solenoid output module
static inline uv_output_state_e uv_dual_solenoid_output_get_state(uv_dual_solenoid_output_st *this,
		uv_dual_solenoid_output_solenoids_e solenoid) {
	return uv_solenoid_output_get_state(&this->solenoid[solenoid]);
}



/// @brief: Sets the mode for this output. Mode is either PROP or ONOFF.
static inline void uv_dual_solenoid_output_set_mode(uv_dual_solenoid_output_st *this,
		uv_prop_output_modes_e value) {
	uv_prop_output_set_mode((uv_prop_output_st *) this, value);
}



static inline uv_prop_output_modes_e uv_dual_solenoid_output_get_mode(
		uv_dual_solenoid_output_st *this) {
	return uv_prop_output_get_mode((uv_prop_output_st *) this);
}







static inline void uv_dual_solenoid_output_set_enable_delays_ms(
		uv_dual_solenoid_output_st *this, uint32_t pre_delay, uint32_t post_delay) {
	uv_prop_output_set_enable_delays_ms((uv_prop_output_st*) this, pre_delay, post_delay);
}





static inline void uv_dual_solenoid_output_set_dither_freq(uv_dual_solenoid_output_st *this,
		uint16_t value) {
	uv_solenoid_output_set_dither_freq(&this->solenoid[0], value);
	uv_solenoid_output_set_dither_freq(&this->solenoid[1], value);
}



static inline void uv_dual_solenoid_output_set_dither_ampl(uv_dual_solenoid_output_st *this,
		uint16_t value) {
	uv_solenoid_output_set_dither_ampl(&this->solenoid[0], value);
	uv_solenoid_output_set_dither_ampl(&this->solenoid[1], value);
}



static inline void uv_dual_solenoid_output_set_maxspeed_scaler(
		uv_dual_solenoid_output_st *this, int16_t value) {
	uv_prop_output_set_maxspeed_scaler((uv_prop_output_st*) this, value);
}



static inline int16_t uv_dual_solenoid_output_get_maxspeed_scaler(
		uv_dual_solenoid_output_st *this) {
	return uv_prop_output_get_maxspeed_scaler((uv_prop_output_st*) this);
}





#endif

#endif /* UV_HAL_INC_OUTPUT_UV_DUAL_SOLENOID_OUTPUT_H_ */
