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


#include "uv_solenoid_output.h"


#if CONFIG_SOLENOID_OUTPUT

#define OPENLOOP_DELAY_MS		500


static uint16_t current_func(void *this_ptr, uint16_t adc) {
	int32_t current = (int32_t) adc * ((uv_output_st*) this_ptr)->sense_ampl / 1000;
	uv_solenoid_output_st *this = this_ptr;

	// apply pwm duty cycle compensation
	// vnd5050 current feedback is filtered with a strong low-pass filter.
	// It cannot follow PWM signal strongly, and thus the actual current would be get
	// if the adc would be sampled when the PWM output was active (high). As this kind
	// of synchronization is not available, we measure the average current and
	// compensate the PWM output **off** time out from the value
	int32_t pwmdc = uv_moving_aver_get_val(&this->pwmaver);
	if (pwmdc > 10) {
		// Clamp the divisor. The duty cycle average follows the PWM output far
		// faster than the hardware current sense filter follows the actual
		// current, so right after the duty cycle drops the compensation divides
		// an already measured high current with an already dropped duty cycle.
		// Without a clamp that briefly reports a current many times greater than
		// what can physically flow, which shows up as a false fault trip and as
		// a big erroneous error value for the current PID controller.
		if (pwmdc < SOLENOID_OUTPUT_PWM_COMP_MIN_DC) {
			pwmdc = SOLENOID_OUTPUT_PWM_COMP_MIN_DC;
		}
		current = current * PWM_MAX_VALUE / pwmdc;
	}
	// saturate rather than wrap around, the return value is 16-bit
	LIMIT_MAX(current, UINT16_MAX);

	return current;
}


void uv_solenoid_output_conf_reset(uv_solenoid_output_conf_st *conf,
		uv_solenoid_output_limitconf_st *limitconf) {
	conf->min = 0;
	conf->max = SOLENOID_OUTPUT_CONF_MAX;
	limitconf->max = CONFIG_SOLENOID_MAX_CURRENT_DEF;
	limitconf->min = 0;
}


void uv_solenoid_output_init(uv_solenoid_output_st *this,
		uv_solenoid_output_conf_st *conf_ptr, uv_solenoid_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uint16_t dither_freq, int16_t dither_ampl,
		uv_adc_channels_e adc_chn, uint16_t sense_ampl,
		uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_openloop, uint32_t emcy_fault) {

	this->conf = conf_ptr;
	this->limitconf = limitconf;

	uv_output_init(((uv_output_st*) this), adc_chn, 0, sense_ampl, max_current,
			fault_current, SOLENOID_OUTPUT_MAAVG_COUNT, emcy_openloop, emcy_fault);
	uv_output_set_current_func(((uv_output_st*) this), &current_func);

	uv_moving_aver_init(&this->pwmaver, SOLENOID_OUTPUT_PWMAVG_COUNT);

	this->mode = SOLENOID_OUTPUT_MODE_CURRENT;

	this->maxspeed_scaler = 1000;

	this->dither_ampl = dither_ampl;
	if (dither_freq) {
		this->dither_ms = 1000 / (dither_freq * 2);
		uv_delay_init(&this->delay, this->dither_ms);
	}
	else {
		this->dither_ms = 0;
	}
	this->target = 0;
	this->out = 0;
	this->pwm = 0;
	this->pwm_chn = pwm_chn;
	uv_pwm_set(this->pwm_chn, this->pwm);

	this->force_set = false;

	uv_delay_init(&this->openloop_delay, OPENLOOP_DELAY_MS);

	this->driver_fault_count = 0;
	uv_delay_end(&this->fault_delay);

	uv_pid_init(&this->ma_pid, CONFIG_SOLENOID_MA_P, CONFIG_SOLENOID_MA_I, 0);

}


void uv_solenoid_output_step(uv_solenoid_output_st *this, uint16_t step_ms) {
	uv_output_step((uv_output_st *)this, step_ms);

	LIMITS(this->maxspeed_scaler, 0, 1000);
	LIMIT_MAX(this->conf->max, SOLENOID_OUTPUT_CONF_MAX);
	LIMIT_MAX(this->conf->min, this->conf->max);
	LIMIT_MAX(this->limitconf->max, this->mode == SOLENOID_OUTPUT_MODE_PWM ?
			1000 : CONFIG_SOLENOID_MAX_CURRENT_DEF);
	LIMIT_MAX(this->limitconf->min, this->limitconf->max);

	// Cooldown after a driver fault. OUTPUT_STATE_FAULT can only be left
	// through OUTPUT_STATE_OFF, so holding the OFF transition back here keeps
	// the output shut down. Together with the target having to reach zero it
	// means a driver fault needs the request to be released and the cooldown to
	// expire before the output can be driven again, instead of retrying
	// straight back into a driver that is already against a hardware limit.
	uv_delay(&this->fault_delay, step_ms);

	// set output to OFF state when target is zero and either PWM or ADC value is zero.
	// This disables the ADC current measuring, even when there's open load.
	if ((!!this->target == 0) &&
			((this->pwm == 0) ||
					((uv_solenoid_output_get_current(this) == 0) && !this->force_set)) &&
			uv_delay_has_ended(&this->fault_delay)) {
		uv_solenoid_output_set_state(this, OUTPUT_STATE_OFF);
	}
	else {
		uv_solenoid_output_set_state(this, OUTPUT_STATE_ON);
	}


	uv_output_state_e state = uv_solenoid_output_get_state(this);
	if (state != OUTPUT_STATE_ON) {
		// output is off for any reason
		this->target = 0;
		// reset PID controller just to make sure that it is always initialized correctly
		uv_pid_init(&this->ma_pid, CONFIG_SOLENOID_MA_P, CONFIG_SOLENOID_MA_I, 0);
		// make sure dither doesn't remain in the output
		if (this->dither_ampl > 0) {
			this->dither_ampl *= -1;
		}
		this->pwm = 0;
		uv_delay_init(&this->openloop_delay, OPENLOOP_DELAY_MS);
		uv_moving_aver_reset(&this->pwmaver);
	}
	else {
		// output is ON
		if (this->dither_ms &&
				uv_delay(&this->delay, step_ms)) {
			// toggle dither
			this->dither_ampl *= -1;
			uv_delay_init(&this->delay, this->dither_ms);
		}

		int32_t output = 0;

		LIMIT_MAX(this->target, 1000);

		// solenoid is current driven
		if (this->mode == SOLENOID_OUTPUT_MODE_CURRENT) {
			// set the target current for the pid
			int16_t target_ma = 0;
			// clamp the output current to min & max current limits
			if (this->target) {
				int32_t rel = uv_reli(this->conf->min, 0, UINT8_MAX);
				int32_t maxspeed = uv_lerpi(
						this->maxspeed_scaler,
						this->conf->min,
						this->conf->max);
				// convert maxspeed to 0...1000 scale
				maxspeed = uv_reli(maxspeed, 0, SOLENOID_OUTPUT_CONF_MAX);
				int32_t min_ma = uv_lerpi(rel, this->limitconf->min, this->limitconf->max),
						max_ma = uv_lerpi(maxspeed, this->limitconf->min, this->limitconf->max);
				target_ma = uv_lerpi(this->target, min_ma, max_ma);
				LIMIT_MAX(target_ma, this->limitconf->max);
			}
			uv_pid_set_target(&this->ma_pid, target_ma);


			// milliamp PID controller
			// we calculate current by ourselves because uv_output_st adds averaging
			// which we dont need here. Average value should only be shown to the end user
			// to make an assumption that the current measurement is precise
			uint16_t adc = uv_adc_read(((uv_output_st*) this)->adc_chn);
			// Raw, uncompensated sense reading. The high side driver reports
			// overtemperature and overcurrent by forcing a large current into
			// its current sense output, so the fault flag has to be recognised
			// from the raw reading, before the pwm compensation scales it by a
			// duty cycle dependent amount.
			int32_t raw_ma = (int32_t) adc *
					((uv_output_st*) this)->sense_ampl / 1000;
			uint16_t current = ((uv_output_st*) this)->current_func(this, adc);

			// The load cannot produce a raw reading above the current the loop
			// is limited to, so anything above that level is the driver
			// reporting overtemperature or overcurrent through its current
			// sense output. Shut the output down: a high side driver held
			// against a hardware limit destroys itself.
			int32_t fault_level = (int32_t) this->limitconf->max *
					SOLENOID_OUTPUT_DRIVER_FAULT_NUM /
					SOLENOID_OUTPUT_DRIVER_FAULT_DEN;
			if (raw_ma > fault_level) {
				if (this->driver_fault_count < UINT16_MAX) {
					this->driver_fault_count++;
				}
			}
			else {
				this->driver_fault_count = 0;
			}
			bool driver_fault =
					(this->driver_fault_count >= SOLENOID_OUTPUT_DRIVER_FAULT_CNT);

			uint16_t pwm_get = uv_pwm_get(this->pwm_chn);
			// error sum from before this step, restored by the anti-windup below
			int32_t pid_sum = uv_pid_get_sum(&this->ma_pid);

			if (driver_fault) {
				// Cut the drive in this same step cycle rather than letting the
				// PID react to the fault level as if it were a measurement.
				// OUTPUT_STATE_FAULT sends the fault EMCY and latches, and the
				// cooldown below keeps it down until the request is released.
				output = 0;
				this->pwm = 0;
				uv_pwm_set(this->pwm_chn, 0);
				uv_delay_init(&this->fault_delay,
						SOLENOID_OUTPUT_DRIVER_FAULT_COOLDOWN_MS);
				uv_output_set_state((uv_output_st*) this, OUTPUT_STATE_FAULT);
			}
			else if (target_ma == 0) {
				// Nothing is asked from this output, release it in this same step
				// cycle instead of letting the PID walk the duty cycle down.
				//
				// The PID output is applied as an increment to the duty cycle, so
				// the duty cycle only comes down while the error stays negative.
				// With no load there is no current to produce that error: driving
				// an open load integrates the error sum up until the anti-windup
				// below bounds it, and when the target is then removed the error
				// is zero, leaving the remaining I term to hold the duty cycle at
				// the rail. The output was released only once the heavily averaged
				// current measurement had decayed to zero, which takes hundreds of
				// milliseconds. As uv_dual_solenoid_output waits for the other
				// direction's duty cycle to reach zero before it energizes the
				// opposite solenoid, changing the direction of an output with
				// nothing connected to it was delayed for that same time.
				uv_pid_init(&this->ma_pid, CONFIG_SOLENOID_MA_P, CONFIG_SOLENOID_MA_I, 0);
				output = 0;
			}
			else {
				uv_pid_step(&this->ma_pid, step_ms, current);

				output = (int32_t) pwm_get +
					uv_pid_get_output(&this->ma_pid) +
					this->dither_ampl / 2;
			}


			// Anti-windup. The PID output is applied as an increment to the PWM
			// duty cycle, so whenever the requested duty cycle saturates, the
			// increment is silently discarded but the error stays integrated.
			// Discard the error that was just integrated when it would only
			// drive the output further into the saturation it is already in.
			//
			// Without this the error sum keeps growing for as long as the duty
			// cycle is stuck at a rail. Once the current finally reaches the
			// target and the error changes sign, the accumulated sum first holds
			// the output at the rail and then slams it to the opposite one, and
			// the output ends up oscillating between full current and zero at
			// roughly 1 Hz. It is triggered by target currents that need a duty
			// cycle close to the maximum, which is where the loop saturates.
			int32_t err = (int32_t) target_ma - (int32_t) current;
			if (((output > PWM_MAX_VALUE) && (err > 0)) ||
					((output < 0) && (err < 0))) {
				uv_pid_set_sum(&this->ma_pid, pid_sum);
			}
			else {

			}

			// Open loop detection. The output asks for the maximum duty cycle
			// but no current flows. Note that this cannot be detected from the
			// magnitude of the PID output, as the anti-windup above keeps the
			// error sum bounded.
			if ((output >= PWM_MAX_VALUE) &&
					(abs(uv_output_get_current((uv_output_st*) this)) < 20)) {
				if (uv_delay(&this->openloop_delay, step_ms)) {
					// pid seems to be unable to drive to the target value.
					// This indicates open loop
					uv_output_set_state((uv_output_st*) this, OUTPUT_STATE_OPENLOOP);
				}
			}
			else {
				uv_delay_init(&this->openloop_delay, OPENLOOP_DELAY_MS);
			}


		}
		// solenoid is PWM driven
		else if (this->mode == SOLENOID_OUTPUT_MODE_PWM) {
			if (this->target) {
				// min and max are 0 ... 1000
				int32_t min = uv_reli(this->conf->min, 0, UINT8_MAX);
				int32_t max = uv_reli(this->conf->max, 0, UINT8_MAX);
				output = uv_lerpi(this->target,
						uv_lerpi(min, this->limitconf->min, this->limitconf->max),
						uv_lerpi(
								uv_lerpi(this->maxspeed_scaler, min, max),
								uv_lerpi(min, this->limitconf->min, this->limitconf->max),
								this->limitconf->max));
			}
		}
		// solenoid is on/off
		else { // SOLENOID_OUTPUT_MODE_ONOFF
			if (this->target) {
				output = PWM_MAX_VALUE;
			}
		}
		LIMITS(output, 0, PWM_MAX_VALUE);

		if (!this->force_set) {
			// set the output value
			this->pwm = output;
		}
		// set output state depending if the output is active
		uv_output_set_state((uv_output_st *) this, (output) ? OUTPUT_STATE_ON : OUTPUT_STATE_OFF);
	}

	// set the output pwm
	uv_pwm_set(this->pwm_chn, this->pwm);
	// update pwm avg value
	uv_moving_aver_step(&this->pwmaver, this->pwm);

	// update out variable
	switch (this->mode) {
	case SOLENOID_OUTPUT_MODE_PWM:
		this->out = this->pwm;
		break;
	case SOLENOID_OUTPUT_MODE_CURRENT:
	case SOLENOID_OUTPUT_MODE_ONOFF:
	default:
		this->out = (state == OUTPUT_STATE_ON) ?
				uv_output_get_current((uv_output_st*) this) : 0;
		break;
	}


	this->force_set = false;

}


void uv_solenoid_output_disable(uv_solenoid_output_st *this) {
	uv_output_disable((uv_output_st *) this);
	uv_pwm_set(this->pwm_chn, 0);
}


void uv_solenoid_output_set_dither_ampl(
		uv_solenoid_output_st *this, int16_t ampl) {
	if (abs(this->dither_ampl) != ampl) {
		this->dither_ampl = ampl;
	}
}


void uv_solenoid_output_force_set_pwm(uv_solenoid_output_st *this, uint16_t pwm) {
	this->force_set = true;
	this->pwm = pwm;
}


#endif

