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


#ifndef UV_HAL_INC_UV_PROP_OUTPUT_H_
#define UV_HAL_INC_UV_PROP_OUTPUT_H_



#include <uv_hal_config.h>
#include "uv_solenoid_output.h"


#if CONFIG_PROP_OUTPUT

#if !defined(CONFIG_PROP_OUTPUT_ACC_DEF)
#error "CONFIG_PROP_OUTPUT_ACC_DEF should define the default value for acceleration factor, 0 ... INT8_MAX"
#endif
#if !defined(CONFIG_PROP_OUTPUT_ACC_DEF)
#error "CONFIG_PROP_OUTPUT_ACC_DEF should define the default value for acceleration factor, 0 ... INT8_MAX"
#endif





/// @brief: Defines the prop output modes. These are different than solenoid output modes;
/// Solenoid output can be defined as current or PWM, but prop output tries to stay out
/// of such low-level stuff, being only proportional or toggle.
typedef enum {
	PROP_OUTPUT_MODE_PROP_NORMAL = 0,
	PROP_OUTPUT_MODE_PROP_TOGGLE,
	PROP_OUTPUT_MODE_ONOFF_NORMAL,
	PROP_OUTPUT_MODE_ONOFF_TOGGLE
} uv_prop_output_modes_e;


#define PROP_ACC_MAX							INT8_MAX
#define PROP_ACC_MIN							20
#define PROP_DEC_MAX							INT8_MAX
#define PROP_DEC_MIN							20
#define PROP_VALUE_MAX							INT8_MAX
#define PROP_VALUE_MIN							(INT8_MIN + 1)

#define PROP_OUTPUT_A_MIN_PPT_SUBINDEX			SOLENOID_OUTPUT_MIN_PPT_SUBINDEX
#define PROP_OUTPUT_A_MAX_PPT_SUBINDEX			SOLENOID_OUTPUT_MAX_PPT_SUBINDEX
#define PROP_OUTPUT_B_MIN_PPT_SUBINDEX			(SOLENOID_OUTPUT_CONF_SUBINDEX_COUNT + SOLENOID_OUTPUT_MIN_PPT_SUBINDEX)
#define PROP_OUTPUT_B_MAX_PPT_SUBINDEX			(SOLENOID_OUTPUT_CONF_SUBINDEX_COUNT + SOLENOID_OUTPUT_MAX_PPT_SUBINDEX)
#define PROP_OUTPUT_ACC_SUBINDEX				(SOLENOID_OUTPUT_CONF_SUBINDEX_COUNT * 2 + 1)
#define PROP_OUTPUT_DEC_SUBINDEX				(SOLENOID_OUTPUT_CONF_SUBINDEX_COUNT * 2 + 2)
#define PROP_OUTPUT_TOGGLE_THRESHOLD_DEFAULT		(INT8_MAX / 2)
#define PROP_OUTPUT_TOGGLE_LIMIT_MS_DEFAULT		0
#define PROP_OUTPUT_ENABLE_DELAY_MS_DEFAULT		0

/// @brief: Configuration structure for the dual solenoid module
typedef struct {
	//NOTE: All of these variables should
	uv_solenoid_output_conf_st solenoid_conf[2];
	/// @brief: Control value acceleration factor, from 0 ... 100
	uint8_t acc;
	/// @brief: Control value deceleration factor, from 0 ... 100
	uint8_t dec;
} uv_prop_output_conf_st;



typedef struct {
	uv_solenoid_output_limitconf_st solenoid_limitconf[2];
} uv_prop_output_limitconf_st;



/// @brief: Resets the dual solenoid output configuration module to default settings
void uv_prop_output_conf_reset(uv_prop_output_conf_st *this,
		uv_prop_output_limitconf_st *limitconf);


/// @brief: Dual solenoid output module. Works as a data structure for controlling dual
/// direction proportional valves.
typedef struct {

	// parameter configurations
	uv_prop_output_conf_st *conf;

	uv_prop_output_limitconf_st *limitconf;

	uv_output_state_e state;

	// the requested target output value from -1000 to 1000, the actual drive current is
	// based on solenoid output configurations.
	int16_t target_req;
	int16_t last_target_req;
	// the actual target value. This is smoothened with acc and dec.
	int16_t target;
	// Helper variable to hold more precise target value. With this
	// the PID calculations are done with greater precision than the output actually is
	int16_t target_mult;
	uv_pid_st target_pid;
	uv_delay_st target_delay;
	uv_prop_output_modes_e mode;

	uv_hysteresis_st toggle_hyst;
	uint8_t last_hyst;
	uint8_t toggle_threshold;
	// tells the output state on ONOFFTOGGLE modes
	int8_t toggle_on;
	uv_delay_st toggle_delay;

	uint32_t toggle_limit_ms_pos;
	uint32_t toggle_limit_ms_neg;
	uint32_t enable_pre_delay_ms;
	uint32_t enable_post_delay_ms;
	uv_delay_st pre_enable_delay;
	uint8_t pre_enable_dir;
	uv_delay_st post_enable_delay;
	int16_t post_enable_val;

	int16_t maxspeed_scaler;

} uv_prop_output_st;




/// @brief: Initializes the dual solenoid output module
void uv_prop_output_init(uv_prop_output_st *this,
		uv_prop_output_conf_st *conf, uv_prop_output_limitconf_st *limitconf);


/// @brief: Dual solenoid step function
void uv_prop_output_step(uv_prop_output_st *this, uint16_t step_ms);



void uv_prop_output_clear(uv_prop_output_st *this);


static inline uv_output_state_e uv_prop_output_get_state(uv_prop_output_st *this) {
	return this->state;
}

void uv_prop_output_enable(uv_prop_output_st *this);

static inline void uv_prop_output_disable(uv_prop_output_st *this) {
	this->state = OUTPUT_STATE_DISABLED;
}


#define PROP_OUTPUT_TARGET_MAX			1000
#define PROP_OUTPUT_TARGET_MIN			-1000
/// @brief: Sets the output current.
///
/// @param value: current in -1000 ... 1000 which is set to the actual solenoid output modules
static inline void uv_prop_output_set(uv_prop_output_st *this, int16_t value) {
	this->target_req = value;
}

/// @brief: Returns the target value, in -1000 ... 1000
static inline int16_t uv_prop_output_get_target(uv_prop_output_st *this) {
	return this->target;
}


/// @brief: Copies the configuration settings to the module
static inline void uv_prop_output_set_conf(uv_prop_output_st *this,
					uv_prop_output_conf_st *conf) {
	this->conf = conf;
}


/// @brief: Returns the configuration parameter structure
static inline uv_prop_output_conf_st *uv_prop_output_get_conf(uv_prop_output_st *this) {
	return this->conf;
}

/// @brief: Returns the limit configuration parameter structure
static inline uv_prop_output_limitconf_st *uv_prop_output_get_limitconf(uv_prop_output_st *this) {
	return this->limitconf;
}



/// @brief: Sets the mode for both outputs. The mode can be either current oŕ pwm.
/// The B output is updated in the prop_output_step function according to A output.
static inline void uv_prop_output_set_mode(uv_prop_output_st *this,
		uv_prop_output_modes_e value) {
	this->mode = value;
}

static inline uv_prop_output_modes_e uv_prop_output_get_mode(
		uv_prop_output_st *this) {
	return this->mode;
}


static inline void uv_prop_output_set_maxspeed_scaler(
		uv_prop_output_st *this, int16_t value) {
	this->maxspeed_scaler = value;
}


static inline int16_t uv_prop_output_get_maxspeed_scaler(
		uv_prop_output_st *this) {
	return this->maxspeed_scaler;
}


static inline void uv_prop_output_set_toggle_threshold(
		uv_prop_output_st *this, int8_t value) {
	this->toggle_threshold = abs(value);
}


static inline void uv_prop_output_set_toggle_limit_ms(
		uv_prop_output_st *this, uint32_t value_pos, uint32_t value_neg) {
	this->toggle_limit_ms_pos = value_pos;
	this->toggle_limit_ms_neg = value_neg;
}


/// @brief: Sets the enable delays time in milliseconds. The output
/// is set ON only after the request has been active for pre delay amount of time,
/// and OFF after the request has been off for post delay amount of time
static inline void uv_prop_output_set_enable_delays_ms(
		uv_prop_output_st *this, uint32_t pre_delay, uint32_t post_delay) {
	this->enable_pre_delay_ms = pre_delay;
	this->enable_post_delay_ms = post_delay;
}


/// @brief: Returns the direction of ONOFFTOGGLE mode state. -1 if negative direction is active,
/// 1 if positive is active, 0 otherwise.
static inline uint8_t uv_prop_output_get_onofftoggle_dir(uv_prop_output_st *this) {
	return this->toggle_on;
}




#endif



#endif

