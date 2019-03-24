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


#ifndef UV_HAL_INC_UV_SOLENOID_OUTPUT_H_
#define UV_HAL_INC_UV_SOLENOID_OUTPUT_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_output.h"
#include "uv_pid.h"
#include "uv_pwm.h"


#if CONFIG_SOLENOID_OUTPUT

#define SOLENOID_OUTPUT_PWMAVG_COUNT	10
#define SOLENOID_OUTPUT_MAAVG_COUNT		100

#if !CONFIG_PID
#error "uv_solenoid_output requires uv_pid_st to be enabled with CONFIG_PID defined as 1."
#endif
#if !CONFIG_PWM
#error "uv_solenoid_output requires CONFIG_PWM to be enabled."
#endif


#if !defined(CONFIG_SOLENOID_MA_P)
#error "CONFIG_SOLENOID_MA_P should define the P factor for load resistance measurement."
#endif
#if !defined(CONFIG_SOLENOID_MA_I)
#error "CONFIG_SOLENOID_MA_I should define the I factor for load resistance measurement."
#endif
#if !defined(CONFIG_SOLENOID_MAX_CURRENT_DEF)
#error "CONFIG_SOLENOID_MAX_CURRENT_DEF should define the default maximum current value for solenoid output in mA."
#endif
#if !defined(CONFIG_SOLENOID_MIN_CURRENT_DEF)
#error "CONFIG_SOLENOID_MIN_CURRENT_DEF should define the default minimum current value for solenoid output in mA."
#endif
#if !defined(CONFIG_SOLENOID_MODE_PWM)
#error "CONFIG_SOLENOID_MODE_PWM should define if the PWM mode and it's configuration settings are enabled"
#endif
#if CONFIG_SOLENOID_MODE_PWM
#if !defined(CONFIG_SOLENOID_MIN_PPT_DEF)
#error "CONFIG_SOLENOID_MIN_PPT_DEF should define the min ppt default value in PWM mode"
#endif
#if !defined(CONFIG_SOLENOID_MAX_PPT_DEF)
#error "CONFIG_SOLENOID_MAX_PPT_DEF should define the max ppt default value in PWM mode"
#endif
#endif
#if !defined(CONFIG_SOLENOID_MODE_ONOFF)
#error "CONFIG_SOLENOID_MODE_ONOFF should define if the ONOFF mode and it's configuration settings are enabled"
#endif
#if CONFIG_SOLENOID_MODE_ONOFF
#if !defined(CONFIG_SOLENOID_ONOFF_MODE_DEF)
#error "CONFIG_SOLENOID_ONOFF_MODE_DEF should define the default state for ONOFF mode, either \
SOLENOID_OUTPUT_ONOFF_MODE_NORMAL or SOLENOID_OUTPUT_ONOFF_MODE_TOGGLE."
#endif
#endif

typedef enum {
	/// @brief: Output is current controlled with a current sensing feedback
	SOLENOID_OUTPUT_MODE_CURRENT,
	/// @brief: Output is raw PWM output, controlled with duty cycle. Note that with this
	/// mode the conf settings are straightly converted to PWM duty cycle values from 0 ... 1000.
	SOLENOID_OUTPUT_MODE_PWM,
	/// @brief: The output is on/off digital output, without any proportional functionality.
	SOLENOID_OUTPUT_MODE_ONOFF
} uv_solenoid_output_mode_st;


#define SOLENOID_OUTPUT_MIN_MA_SUBINDEX			1
#define SOLENOID_OUTPUT_MAX_MA_SUBINDEX			2
#define SOLENOID_OUTPUT_MIN_PPT_SUBINDEX		3
#define SOLENOID_OUTPUT_MAX_PPT_SUBINDEX		4
#define SOLENOID_OUTPUT_ONOFF_MODE_SUBINDEX		5

#if CONFIG_SOLENOID_MODE_PWM
#define _SOLENOID_PWM_SUBINDEX_COUNT			2
#else
#define _SOLENOID_PWM_SUBINDEX_COUNT			0
#endif
#if CONFIG_SOLENOID_MODE_ONOFF
#define _SOLENOID_ONOFF_SUBINDEX_COUNT			1
#else
#define _SOLENOID_ONOFF_SUBINDEX_COUNT			0
#endif
#define SOLENOID_OUTPUT_CONFIG_SUBINDEX_COUNT	(2 + \
		_SOLENOID_PWM_SUBINDEX_COUNT + \
		_SOLENOID_ONOFF_SUBINDEX_COUNT)


enum {
	SOLENOID_OUTPUT_ONOFF_MODE_NORMAL = 0,
	SOLENOID_OUTPUT_ONOFF_MODE_TOGGLE
};
typedef uint16_t uv_solenoid_output_onoff_mode_e;


/// @brief: Data structure for solenoid output configuration data.
/// This can be stored in non-volatile memory.
typedef struct {
	// minimum current in positive direction in milliamps in current mode,
	// min pwm value in pwm mode
	uint16_t min_ma;
	// maximum current in positive direction in milliamps in current mode,
	// max pwm value in pwm mode
	uint16_t max_ma;
#if CONFIG_SOLENOID_MODE_PWM
	// minimum PWM ppt, equals to min_ma in current mode
	uint16_t min_ppt;
	// maximum PWM ppt, equals to max_ma in current mode
	uint16_t max_ppt;
#endif
#if CONFIG_SOLENOID_MODE_ONOFF
	// defines if the output is toggleable. That is, the output is left ON
	// waiting for another press to set it off.
	uint16_t onoff_mode;
#endif
} uv_solenoid_output_conf_st;

/// @brief: Resets the output values to defaults
void uv_solenoid_output_conf_reset(uv_solenoid_output_conf_st *conf);



typedef struct {
	EXTENDS(uv_output_st);

	uv_solenoid_output_mode_st mode;

	// solenoid configuration parameters
	uv_solenoid_output_conf_st *conf;

	/// @brief: Dither time cycle (1 / frequency)
	uint16_t dither_ms;
	/// @brief: Dither amplitude, measured in pwm scale. See uv_pwm.h for more details.
	int16_t dither_ampl;
	/// @brief: Dither delay
	uv_delay_st delay;
	/// @brief: PID controller for controlling the current. Has to be as fast as possible
	uv_pid_st ma_pid;
	/// @brief: Target value from 0 ... 1000. This is scaled to the output value
	/// depending on the output mode.
	uint16_t target;
	/// @brief: Stores the current PWM duty cycle
	uint16_t pwm;
	/// @brief: Stores the output value on every specific moment. If the
	/// solenoid output mode is current or onoff, this is in milliamps, if the mode
	/// is PWM, the output is in ppt.
	uint16_t out;
	/// @brief: Stores the average PWM duty cycle value, used for compensation in
	/// calculating the current
	uv_moving_aver_st pwmaver;
	/// @brief: PWM channel configured for this output
	uv_pwm_channel_t pwm_chn;
	// stores the last target value. Useful i.e. in ONOFF_TOGGLE mode
	uint16_t last_target;

} uv_solenoid_output_st;


/// @brief: Initializes the solenoid output
///
/// @param dither_freq: The frequency of super imposed dither in Hz
/// @param dither_ampl: The amplitude of super imposed dither in pwm duty cycle, i.e. promilles.
/// @param adc_chn: Current sense feedback analog channel
/// @param sense_ampl: Amplification for current sense feedback. ADC value
/// from current sense feedback is multiplied with this in order to get milliamps.
/// @param max_current: Maximum allowed current, which is used to detect overload situations.
/// @param fault_current: Current limit of which greater current is indicated as fault situation.
/// Should always be greater than **max_current**.
/// @param moving_avg_count: Count for current sense moving average filter.
/// @emcy_overload: CANopen EMCY message for overload situation
// @emcy_fault: CANopen EMCY message for fault situation
void uv_solenoid_output_init(uv_solenoid_output_st *this,
		uv_solenoid_output_conf_st *conf_ptr, uv_pwm_channel_t pwm_chn,
		uint16_t dither_freq, int16_t dither_ampl, uv_adc_channels_e adc_chn,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload, uint32_t emcy_fault);


/// @brief: Sets the output mode. Defaults to current controlled.
static inline void uv_solenoid_output_set_mode(uv_solenoid_output_st *this,
		uv_solenoid_output_mode_st mode) {
	this->mode = mode;
}

/// @brief: Returns the solenoid output mode
static inline uv_solenoid_output_mode_st uv_solenoid_output_get_mode(uv_solenoid_output_st *this) {
	return this->mode;
}


/// @brief: Step funtion
void uv_solenoid_output_step(uv_solenoid_output_st *this, uint16_t step_ms);


/// @brief: Sets the solenoid output target value. In current mode this is scaled to milliamps
/// according to configuration values,
/// in PWM mode the duty cycle.
/// Values should be 0 ... 1000.
void uv_solenoid_output_set(uv_solenoid_output_st *this, uint16_t value);


/// @brief: Returns the output value. The unit of the value depends on the mode of
/// the solenoid output. In current and onoff mode, output current is returned in milliamps,
/// and in pwm mode the ppt is returned.
static inline uint16_t uv_solenoid_output_get_out(uv_solenoid_output_st *this) {
	return this->out;
}

/// @brief: Sets the output state
static inline void uv_solenoid_output_set_state(uv_solenoid_output_st *this,
		uv_output_state_e state) {
	uv_output_set_state((uv_output_st *) this, state);
}


/// @brief: Disables the output. Output can be enabled only by calling
/// *uv_solenoid_output_enable*.
void uv_solenoid_output_disable(uv_solenoid_output_st *this);


/// @brief: Enabled the output once it's disabled with *uv_solenoid_output_disable*.
static inline void uv_solenoid_output_enable(uv_solenoid_output_st *this) {
	uv_output_enable((uv_output_st *) this);
}


/// @brief: returns the state of the output
static inline uv_output_state_e uv_solenoid_output_get_state(
		const uv_solenoid_output_st *this) {
	return uv_output_get_state(((uv_output_st*) this));
}


/// @brief: Returns the current measured from the sense feedback
static inline uint16_t uv_solenoid_output_get_current(uv_solenoid_output_st *this) {
	return uv_output_get_current(((uv_output_st*) this));
}


/// @brief: Copies the configuration parameters to the output
static inline void uv_solenoid_output_set_conf(uv_solenoid_output_st *this,
		uv_solenoid_output_conf_st *conf) {
	this->conf = conf;
}


/// @brief: returns the configuration parameter structure
static inline uv_solenoid_output_conf_st *uv_solenoid_output_get_conf(uv_solenoid_output_st *this) {
	return this->conf;
}


/// @brief: Sets the dither frequency
static inline void uv_solenoid_output_set_dither_freq(
		uv_solenoid_output_st *this, uint16_t freq) {
	this->dither_ms = (1 / (freq * 2));
}


/// @brief: Returns the dither amplitude
static inline int16_t uv_solenoid_output_get_dither_ampl(
		uv_solenoid_output_st *this) {
	return abs(this->dither_ampl);
}


/// @brief: Sets the dither amplitude. Amplitude is in same unit as in uv_pwm.h
static inline void uv_solenoid_output_set_dither_ampl(
		uv_solenoid_output_st *this, int16_t ampl) {
	this->dither_ampl = ampl;
}


/// @brief: Returns the solenoid's current PWM duty cycle value
static inline uint16_t uv_solenoid_output_get_pwm_dc(uv_solenoid_output_st *this) {
	return this->pwm;
}

#endif

#endif /* UV_HAL_INC_UV_SOLENOID_OUTPUT_H_ */
