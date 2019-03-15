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



#include "uv_solenoid_output.h"


#if CONFIG_SOLENOID_OUTPUT


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



void uv_solenoid_output_conf_reset(uv_solenoid_output_conf_st *conf) {
	conf->min_ma = CONFIG_SOLENOID_MIN_CURRENT_DEF;
	conf->max_ma = CONFIG_SOLENOID_MAX_CURRENT_DEF;
#if CONFIG_SOLENOID_MODE_PWM
	conf->min_percent = CONFIG_SOLENOID_MIN_PERCENT_DEF;
	conf->max_percent = CONFIG_SOLENOID_MAX_PERCENT_DEF;
#endif
#if CONFIG_SOLENOID_MODE_ONOFF
	conf->onoff_mode = CONFIG_SOLENOID_ONOFF_MODE_DEF;
#endif
}



void uv_solenoid_output_init(uv_solenoid_output_st *this,
		uv_solenoid_output_conf_st *conf_ptr, uv_pwm_channel_t pwm_chn,
		uint16_t dither_freq, int16_t dither_ampl, uv_adc_channels_e adc_chn,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload, uint32_t emcy_fault) {

	this->conf = conf_ptr;

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
	this->out = 0;
	this->pwm = 0;
	this->pwm_chn = pwm_chn;
	uv_pwm_set(this->pwm_chn, this->pwm);

	uv_pid_init(&this->ma_pid, CONFIG_SOLENOID_MA_P, CONFIG_SOLENOID_MA_I, 0);

}

void uv_solenoid_output_step(uv_solenoid_output_st *this, uint16_t step_ms) {
	uv_output_step((uv_output_st *)this, step_ms);

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

		// solenoid is current driven
		if (this->mode == SOLENOID_OUTPUT_MODE_CURRENT) {
			// set the target current for the pid

			int16_t target_ma = 0;
			// clamp the output current to min & max current limits
			if (this->target) {
				target_ma = uv_lerpi(this->target, this->conf->min_ma, this->conf->max_ma);
				if (target_ma < this->conf->min_ma) {
					target_ma = this->conf->min_ma;
				}
				else if (target_ma > this->conf->max_ma) {
					target_ma = this->conf->max_ma;
				}
				else {

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

			this->out = uv_output_get_current((uv_output_st*) this);
		}
		// solenoid is PWM driven
		else if (this->mode == SOLENOID_OUTPUT_MODE_PWM) {
#if CONFIG_SOLENOID_MODE_PWM
			if (this->target) {
				int32_t rel = uv_reli(this->target, this->conf->min_percent * 10,
						uv_mini(this->conf->max_percent * 10, 1000));
				output = uv_lerpi(rel, this->conf->min_percent * 10,
						uv_mini(this->conf->max_percent * 10, 1000));
			}
			this->out = output;
#endif
		}
		// solenoid is on/off
		else {
#if CONFIG_SOLENOID_MODE_ONOFF
			if (this->conf->onoff_mode == SOLENOID_OUTPUT_ONOFF_MODE_NORMAL) {
				// normal mode
				output = this->target ? 1000 : 0;
			}
			else {
				// toggle mode
				if (this->target && !this->last_target) {
					// target was toggled, toggle the output also
					output = (this->pwm) ? 0 : 1000;
				}
			}
			this->out = uv_output_get_current((uv_output_st*) this);
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

	// update the last target value
	this->last_target = this->target;
}



void uv_solenoid_output_set(uv_solenoid_output_st *this, uint16_t value) {
	this->target = value;
}


void uv_solenoid_output_disable(uv_solenoid_output_st *this) {
	uv_output_disable((uv_output_st *) this);
	uv_pwm_set(this->pwm_chn, 0);
}





#endif

