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




void uv_solenoid_output_init(uv_solenoid_output_st *this, uv_pwm_channel_t pwm_chn,
		uint16_t dither_freq, int16_t dither_ampl, uv_adc_channels_e adc_chn,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload, uint32_t emcy_fault) {
	uv_output_init(((uv_output_st*) this), adc_chn, 0, sense_ampl, max_current,
			fault_current, 1, emcy_overload, emcy_fault);

	this->dither_ampl = dither_ampl;
	if (dither_freq) {
		this->dither_ms = 1000 / (dither_freq * 2);
		uv_delay_init(&this->delay, this->dither_ms);
	}
	else {
		this->dither_ms = 0;
	}
	this->target = 0;
	this->pwm_chn = pwm_chn;
	uv_pwm_set(this->pwm_chn, 0);
	uv_pid_init(&this->ma_pid, CONFIG_SOLENOID_MA_P, CONFIG_SOLENOID_MA_I, 0);

}

void uv_solenoid_output_step(uv_solenoid_output_st *this, uint16_t step_ms) {
	uv_output_step((uv_output_st *)this, step_ms);

	uv_output_state_e state = uv_solenoid_output_get_state(this);
	if ((state == OUTPUT_STATE_OFF) ||
			(state == OUTPUT_STATE_OVERLOAD) ||
			(state == OUTPUT_STATE_FAULT)) {
		// set target to zero
		this->target = 0;
		// make sure dither doesn't remain in the output
		if (this->dither_ampl > 0) {
			this->dither_ampl *= -1;
		}
	}
	else {
		// output is ON

		if (this->dither_ms &&
				uv_delay(&this->delay, step_ms)) {
			// toggle dither
			this->dither_ampl *= -1;
			uv_delay_init(&this->delay, this->dither_ms);
		}
	}

	// set the target value for the pid
	uv_pid_set_target(&this->ma_pid, this->target);

	// milliamp PID controller
	uv_pid_step(&this->ma_pid, step_ms,
			uv_solenoid_output_get_current(this));

	int16_t output = uv_pwm_get(this->pwm_chn) +
			uv_pid_get_output(&this->ma_pid) +
			this->dither_ampl / 2;
	if (output < 0) {
		output = 0;
	}
	// set the output value
	uv_pwm_set(this->pwm_chn, output);
}



void uv_solenoid_output_set(uv_solenoid_output_st *this, uint16_t value_ma) {
	this->target = value_ma;
	uv_solenoid_output_set_state(this,
			value_ma ? OUTPUT_STATE_ON : OUTPUT_STATE_OFF);
}







#endif

