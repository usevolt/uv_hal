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



#include <uv_output.h>


#include <uv_canopen.h>

#if CONFIG_OUTPUT


/// @brief: Sets the output GPIO or PWM accordingly.
/// In OUTPUT_MODE_DIGITAL **value** is evaluated as boolean value,
/// in PWM-based modes value is evaluated as the pwm duty cycle. See uv_pwm.h
/// for more details.
static void set_out(uv_output_st *this, uint16_t value);


void uv_output_init(uv_output_st *this, const output_mode_e mode,
		const uv_adc_channels_e adc_chn, const uint32_t io_pwm,
		const uint16_t sense_mohm, const uint16_t min_val, const uint16_t max_val,
		const uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault) {
	this->mode = mode;
	this->adc_chn = adc_chn;
	this->target_val = 0;
	if (this->mode == OUTPUT_MODE_DIGITAL) {
		this->gate_io = io_pwm;
		uv_gpio_init_output(this->gate_io, false);
	}
	else {
		this->pwm_channel = io_pwm;
	}
	set_out(this, 0);
	this->sense_mohm = sense_mohm;
	this->sense_ampl = 50;
	this->limit_min = min_val;
	this->limit_max = max_val;
	uv_moving_aver_init(&this->moving_avg, moving_avg_count);
	this->emcy_overload = emcy_overload;
	this->emcy_fault = emcy_fault;
	this->state = OUTPUT_STATE_OFF;
	this->current = 0;
	uv_delay_init(&this->dither.delay, CONFIG_OUTPUT_DITHER_FREQ);
	this->dither.addition = 0;
}


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state) {
	if ((this->state != OUTPUT_STATE_FAULT) &&
			(this->state != OUTPUT_STATE_OVERLOAD)) {
#if CONFIG_CANOPEN
		if ((state == OUTPUT_STATE_FAULT) &&
				(this->emcy_fault)) {
			uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_fault);
		}
		else if ((state == OUTPUT_STATE_OVERLOAD) &&
				(this->emcy_overload)) {
			uv_canopen_emcy_send(CANOPEN_EMCY_DEVICE_SPECIFIC, this->emcy_overload);
		}
		else {

		}
#endif
		this->state = state;
	}
	else if (state == OUTPUT_STATE_OFF) {
		this->state = state;
	}
	else {

	}
}


/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms) {

	if (this->state == OUTPUT_STATE_ON) {
		// current sense feedback
		if (this->adc_chn) {
			int16_t adc = uv_adc_read(this->adc_chn) - 0x24;
			if (adc < 0) {
				adc = 0;
			}
			int32_t mv = (int32_t) adc * 3300 / ADC_MAX_VALUE ;
			// current is multiplied by inverted resistor value (1 / 0.002) and divided
			// by current sensing amplification
			int32_t current = mv * (1000 / (this->sense_mohm * this->sense_ampl));
			if (current < 0) {
				current = 0;
			}
			this->current = uv_moving_aver_step(&this->moving_avg, current);
		}

		if ((this->current > this->limit_max)) {
			set_out(this, false);
			uv_output_set_state(this, OUTPUT_STATE_OVERLOAD);
		}
		else if (this->current < this->limit_min) {
			set_out(this, false);
			uv_output_set_state(this, OUTPUT_STATE_FAULT);
		}
		else {
			// normal operation in different modes
			if (this->mode == OUTPUT_MODE_DIGITAL) {
				set_out(this, true);
			}
			else if (this->mode == OUTPUT_MODE_PWM) {
				set_out(this, this->target_val);
			}
			else if (this->mode == OUTPUT_MODE_SOLENOID) {

			}
			else {

			}
		}
	}
	else if (this->state == OUTPUT_STATE_OVERLOAD) {
		set_out(this, false);
	}
	else if (this->state == OUTPUT_STATE_FAULT) {
		set_out(this, false);
	}
	else {
		set_out(this, false);
	}
}


void uv_output_set(uv_output_st *this, const uint16_t value) {
	if (this->mode == OUTPUT_MODE_DIGITAL) {
		uv_output_set_state(this, value ? OUTPUT_STATE_ON : OUTPUT_STATE_OFF);
	}
	else {
		this->target_val = value;
	}
}



static void set_out(uv_output_st *this, uint16_t value) {
	if (this->mode == OUTPUT_MODE_DIGITAL) {
		uv_gpio_set(this->gate_io, value ? true : false);
	}
	else {
#if CONFIG_PWM
		uv_pwm_set(this->pwm_channel, DUTY_CYCLE(value));
#endif
	}
}




#endif
