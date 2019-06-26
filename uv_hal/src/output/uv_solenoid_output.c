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

#define TOGGLE_HYSTERESIS_DEFAULT		100


static uint16_t current_func(void *this_ptr, uint16_t adc) {
	int32_t current = (int32_t) adc * ((uv_output_st*) this_ptr)->sense_ampl / 1000;
	uv_solenoid_output_st *this = this_ptr;

	// apply pwm duty cycle compensation
	// vnd5050 current feedback is filtered with a strong low-pass filter.
	// It cannot follow PWM signal strongly, and thus the actual current would be get
	// if the adc would be sampled when the PWM output was active (high). As this kind
	// of synchronization is not available, we measure the average current and
	// compensate the PWM output **off** time out from the value
	uint16_t pwmdc = uv_moving_aver_get_val(&this->pwmaver);
	if (pwmdc) {
		current = (int32_t) current * PWM_MAX_VALUE / pwmdc;
	}

	return current;
}



void uv_solenoid_output_conf_reset(uv_solenoid_output_conf_st *conf,
		uv_solenoid_output_limitconf_st *limitconf) {
	conf->min_ppt = 100;
	conf->max_ppt = 1000;
	limitconf->max_ma = CONFIG_SOLENOID_MAX_CURRENT_DEF;
	limitconf->max_ppt = CONFIG_SOLENOID_MAX_PPT_DEF;
}



void uv_solenoid_output_init(uv_solenoid_output_st *this,
		uv_solenoid_output_conf_st *conf_ptr, uv_solenoid_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_chn, uint16_t dither_freq, int16_t dither_ampl,
		uv_adc_channels_e adc_chn, uint16_t sense_ampl,
		uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload, uint32_t emcy_fault) {

	this->conf = conf_ptr;
	this->limitconf = limitconf;

	uv_output_init(((uv_output_st*) this), adc_chn, 0, sense_ampl, max_current,
			fault_current, SOLENOID_OUTPUT_MAAVG_COUNT, emcy_overload, emcy_fault);
	uv_output_set_current_func(((uv_output_st*) this), &current_func);

	uv_moving_aver_init(&this->pwmaver, SOLENOID_OUTPUT_PWMAVG_COUNT);

	this->mode = SOLENOID_OUTPUT_MODE_CURRENT;

	this->dither_ampl = dither_ampl;
	if (dither_freq) {
		this->dither_ms = 1000 / (dither_freq * 2);
		uv_delay_init(&this->delay, this->dither_ms);
	}
	else {
		this->dither_ms = 0;
	}
	this->target = 0;
#if CONFIG_SOLENOID_MODE_ONOFF
	uv_hysteresis_init(&this->toggle_hyst,
			SOLENOID_OUTPUT_TOGGLE_THRESHOLD_DEFAULT, TOGGLE_HYSTERESIS_DEFAULT, false);
	this->last_hyst = 0;
	this->toggle_on = false;
	this->toggle_limit_ms = SOLENOID_OUTPUT_TOGGLE_LIMIT_MS_DEFAULT;
	uv_delay_init(&this->toggle_delay, this->toggle_limit_ms);
#endif
	this->out = 0;
	this->pwm = 0;
	this->pwm_chn = pwm_chn;
	uv_pwm_set(this->pwm_chn, this->pwm);

	uv_pid_init(&this->ma_pid, CONFIG_SOLENOID_MA_P, CONFIG_SOLENOID_MA_I, 0);

}

void uv_solenoid_output_step(uv_solenoid_output_st *this, uint16_t step_ms) {
	uv_output_step((uv_output_st *)this, step_ms);

	if (this->conf->min_ppt > this->conf->max_ppt) {
		this->conf->min_ppt = this->conf->max_ppt;
	}
	if (this->conf->max_ppt > 1000) {
		this->conf->max_ppt = 1000;
	}
	if (this->limitconf->max_ppt > 1000) {
		this->limitconf->max_ppt = 1000;
	}

	// set output to OFF state when target is zero and either PWM or ADC value is zero.
	// This disables the ADC current measuring, even when there's open load.
	if ((this->target == 0) &&
			((this->pwm == 0) || (uv_solenoid_output_get_current(this) == 0))) {
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
		this->out = 0;
#if CONFIG_SOLENOID_MODE_ONOFF
		this->toggle_hyst.result = 0;
#endif
	}
	else {
		// output is ON
		if (this->dither_ms &&
				uv_delay(&this->delay, step_ms)) {
			// toggle dither
			this->dither_ampl *= -1;
			uv_delay_init(&this->delay, this->dither_ms);
		}

		int16_t output = 0;

		if (this->target > 1000) {
			this->target = 1000;
		}

		// solenoid is current driven
		if (this->mode == SOLENOID_OUTPUT_MODE_CURRENT) {
			// set the target current for the pid

			int16_t target_ma = 0;
			// clamp the output current to min & max current limits
			if (this->target) {
				target_ma = uv_lerpi(this->target,
						uv_lerpi(this->conf->min_ppt, 0, this->limitconf->max_ma),
						uv_lerpi(this->conf->max_ppt, 0, this->limitconf->max_ma));
				if (target_ma > this->limitconf->max_ma) {
					target_ma = this->limitconf->max_ma;
				}
			}
			uv_pid_set_target(&this->ma_pid, target_ma);

			// milliamp PID controller
			// we calculate current by ourselves because uv_output_st adds averaging
			// which we dont need here. Average value should only be shown to the end user
			// to make an assumption that the current measurement is precise
			uint16_t current = ((uv_output_st*) this)->current_func(this,
					uv_adc_read(((uv_output_st*) this)->adc_chn));
			uv_pid_step(&this->ma_pid, step_ms, current);

			output = uv_pwm_get(this->pwm_chn) +
				uv_pid_get_output(&this->ma_pid) +
				this->dither_ampl / 2;

			// since current_func applies PWM correction, it can lead
			// to a situation where pwm dc is non-zero but target is zero.
			// Here we prevent that from happening.
			if (this->target == 0 &&
					uv_pid_get_output(&this->ma_pid) == 0) {
				output = 0;
			}

			this->out = uv_output_get_current((uv_output_st*) this);
		}
		// solenoid is PWM driven
		else if (this->mode == SOLENOID_OUTPUT_MODE_PWM) {
#if CONFIG_SOLENOID_MODE_PWM
			if (this->target) {
				output = uv_lerpi(this->target,
						uv_lerpi(this->conf->min_ppt, 0, this->limitconf->max_ppt),
						uv_lerpi(this->conf->max_ppt, 0, this->limitconf->max_ppt));
			}
			this->out = output;
#endif
		}
		// solenoid is on/off
		else {
#if CONFIG_SOLENOID_MODE_ONOFF
			uv_hysteresis_step(&this->toggle_hyst, this->target);

			if (this->mode == SOLENOID_OUTPUT_MODE_ONOFF_NORMAL) {
				// normal mode
				if (uv_hysteresis_get_output(&this->toggle_hyst) && this->target) {
					output = 1000;
				}
				else {
					// small trick to prevent bugs when hysteresis > trigger_value
					this->toggle_hyst.result = 0;
				}
				this->out = uv_output_get_current((uv_output_st*) this);
			}
			else {
				// toggle mode
				if (uv_hysteresis_get_output(&this->toggle_hyst) && this->target) {
					if (!this->last_hyst) {
						// target was toggled, toggle the output also
						this->toggle_on = !this->toggle_on;
					}
					uv_delay_init(&this->toggle_delay, this->toggle_limit_ms);
				}
				else {
					// small trick to prevent bugs when hysteresis > trigger_value
					this->toggle_hyst.result = 0;
				}
				// check if the toggle time limit has passed (if enabled)
				// and in that case disable the output
				if (this->toggle_limit_ms > 0 &&
						uv_delay(&this->toggle_delay, step_ms)) {
					this->toggle_on = false;
				}
				this->last_hyst = uv_hysteresis_get_output(&this->toggle_hyst);
				output = (this->toggle_on) ? 1000 : 0;
				this->out = this->toggle_on ? uv_output_get_current((uv_output_st*) this) : 0;
			}
#endif
		}

		if (output < 0) {
			output = 0;
		}
		else if (output > PWM_MAX_VALUE) {
			output = PWM_MAX_VALUE;
		}
		// set the output value
		this->pwm = output;
		// set output state depending if the output is active
		uv_output_set_state((uv_output_st *) this, (output) ? OUTPUT_STATE_ON : OUTPUT_STATE_OFF);
	}

	// set the output pwm
	uv_pwm_set(this->pwm_chn, this->pwm);
	// update pwm avg value
	uv_moving_aver_step(&this->pwmaver, this->pwm);
}



void uv_solenoid_output_set(uv_solenoid_output_st *this, uint16_t value) {
	this->target = value;
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




#endif

