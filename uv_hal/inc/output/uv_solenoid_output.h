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


#define SOLENOID_OUTPUT_CONF_MAX		UINT8_MAX

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

typedef enum {
	/// @brief: Output is current controlled with a current sensing feedback
	SOLENOID_OUTPUT_MODE_CURRENT = 0,
	/// @brief: Output is raw PWM output, controlled with duty cycle. Note that with this
	/// mode the conf settings are straightly converted to PWM duty cycle values from 0 ... 1000.
	SOLENOID_OUTPUT_MODE_PWM,
	/// @brief: The output is on/off digital output, without any proportional functionality.
	SOLENOID_OUTPUT_MODE_ONOFF,
	/// @brief: Output A is PWM single-ended output with middle value in 50% duty cycle,
	/// output B is on/off output for supplying power to Danfoss PVG valve. Note
	/// that uv_solenoid_output is never in this mode, rather uv_dual_solenoid_output is.
	/// Internally uv_dual_solenoid_output sets solenoid outputs to MODE_PWM and MODE_ONOFF modes.
	SOLENOID_OUTPUT_MODE_PVG,
	SOLENOID_OUTPUT_MODE_COUNT
} uv_solenoid_output_mode_e;


#define SOLENOID_OUTPUT_MIN_PPT_SUBINDEX			1
#define SOLENOID_OUTPUT_MAX_PPT_SUBINDEX			2
#define SOLENOID_OUTPUT_CONF_SUBINDEX_COUNT			2



/// @brief: Data structure for solenoid output configuration data.
/// This can be stored in non-volatile memory.
typedef struct {
	// minimum PWM in 0 ... UINT8_MAX
	uint8_t min;
	// maximum PWM in 0 ... UINT8_MAX
	uint8_t max;
} uv_solenoid_output_conf_st;

typedef struct {
	// maximum milliamps / ppt depending on the mode
	uint16_t max;
	// minimum milliamps / ptt depending on the mode
	uint16_t min;
} uv_solenoid_output_limitconf_st;

/// @brief: Resets the output values to defaults
void uv_solenoid_output_conf_reset(uv_solenoid_output_conf_st *conf,
		uv_solenoid_output_limitconf_st *limitconf);



typedef struct {
	EXTENDS(uv_output_st);

	uv_solenoid_output_mode_e mode;

	// solenoid configuration parameters
	uv_solenoid_output_conf_st *conf;

	uv_solenoid_output_limitconf_st *limitconf;

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

	// scales the maximum speed of the solenoid. 0 ... 1000
	int16_t maxspeed_scaler;

	uv_delay_st openloop_delay;

} uv_solenoid_output_st;


/// @brief: Initializes the solenoid output
///
/// @param dither_freq: The frequency of super imposed dither in Hz
/// @param dither_ampl: The amplitude of super imposed dither in pwm duty cycle, i.e. promilles.
/// @param adc_chn: Current sense feedback analog channel
/// @param sense_ampl: Amplification for current sense feedback. ADC value
/// from current sense feedback is multiplied with this in order to get microamperes.
/// @param max_current: Maximum allowed current, which is used to detect overload situations.
/// @param fault_current: Current limit of which greater current is indicated as fault situation.
/// Should always be greater than **max_current**.
/// @param moving_avg_count: Count for current sense moving average filter.
/// @emcy_overload: CANopen EMCY message for overload situation
// @emcy_fault: CANopen EMCY message for fault situation
void uv_solenoid_output_init(uv_solenoid_output_st *this,
		uv_solenoid_output_conf_st *conf_ptr, uv_solenoid_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uint16_t dither_freq, int16_t dither_ampl,
		uv_adc_channels_e adc_chn, uint16_t sense_ampl,
		uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload, uint32_t emcy_fault);


/// @brief: Sets the output mode. Defaults to current controlled.
static inline void uv_solenoid_output_set_mode(uv_solenoid_output_st *this,
		uv_solenoid_output_mode_e mode) {
	this->mode = mode;
}

/// @brief: Returns the solenoid output mode
static inline uv_solenoid_output_mode_e uv_solenoid_output_get_mode(uv_solenoid_output_st *this) {
	return this->mode;
}


/// @brief: Sets the maximum speed scaler value
static inline void uv_solenoid_output_set_maxspeed_scaler(
		uv_solenoid_output_st *this, int16_t value) {
	this->maxspeed_scaler = value;
}


/// @brief: Returns the maximum speed scaler value
static inline int16_t uv_solenoid_output_get_maxspeed_scaler(uv_solenoid_output_st *this) {
	return this->maxspeed_scaler;
}



/// @brief: Step funtion
void uv_solenoid_output_step(uv_solenoid_output_st *this, uint16_t step_ms);



#define SOLENOID_OUTPUT_TARGET_MAX	1000

/// @brief: Sets the solenoid output target value. In current mode range of 0 ... 1000
/// is scaled to milliamps according to configuration values,
/// in PWM mode the duty cycle, in range of 0 ... 1000. The conf values affect the actual duty cycle.
static inline void uv_solenoid_output_set(uv_solenoid_output_st *this, uint16_t value) {
	this->target = value;
}


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



/// @brief: Freezes the fault detection for *ms* given time. This can be used
/// to prevent unintentional faults.
///
/// @note: More than 100 ms freeze is not suggested, as this might burn the whole controller
/// in case of short circuit. The VN5T100 amplifier won't sustain too long short circuit
static inline void uv_solenoid_output_freeze_fault_detection(
		uv_solenoid_output_st *this, uint32_t ms) {
	uv_output_freeze_fault_detection((uv_output_st*) this, ms);
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
	this->dither_ms = (1000 / (freq * 2));
}


/// @brief: Returns the dither amplitude
static inline int16_t uv_solenoid_output_get_dither_ampl(
		uv_solenoid_output_st *this) {
	return abs(this->dither_ampl);
}


/// @brief: Sets the dither amplitude. Amplitude is in same unit as in uv_pwm.h
void uv_solenoid_output_set_dither_ampl(
		uv_solenoid_output_st *this, int16_t ampl);


/// @brief: Returns the solenoid's current PWM duty cycle value
static inline uint16_t uv_solenoid_output_get_pwm_dc(uv_solenoid_output_st *this) {
	return this->pwm;
}






#endif

#endif /* UV_HAL_INC_UV_SOLENOID_OUTPUT_H_ */
