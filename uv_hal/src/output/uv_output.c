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


/// @brief: Sets the output GPIO if one is assigned. When extending
/// different output modules from this, set gate_io to 0.
static inline void set_out(uv_output_st *this, uint16_t value) {
	if (this->gate_io) {
		uv_gpio_set(this->gate_io, value);
	}
}


void uv_output_init(uv_output_st *this,  uv_adc_channels_e adc_chn, uv_gpios_e gate_io,
		uint16_t sense_ampl, uint16_t max_val, uint16_t fault_val,
		uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault) {
	this->adc_chn = adc_chn;
	this->gate_io = gate_io;
	if (this->gate_io) {
		uv_gpio_init_output(this->gate_io, false);
	}
	set_out(this, 0);
	this->sense_ampl = sense_ampl;
	this->limit_max = max_val;
	this->limit_fault = fault_val;
	uv_moving_aver_init(&this->moving_avg, moving_avg_count);
	this->emcy_overload = emcy_overload;
	this->emcy_fault = emcy_fault;
	this->state = OUTPUT_STATE_OFF;
	this->current = 0;
}


/// @brief: Sets the output state
void uv_output_set_state(uv_output_st *this, const uv_output_state_e state) {
	// stat can be changed only if output is not disabled
	if (this->state != OUTPUT_STATE_DISABLED) {
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
}


void uv_output_enable(uv_output_st *this) {
	if (this->state == OUTPUT_STATE_DISABLED) {
		this->state = OUTPUT_STATE_OFF;
	}
}


/// @brief: Step function should be called every step cycle.
void uv_output_step(uv_output_st *this, uint16_t step_ms) {

	// current sense feedback
	if (this->adc_chn) {
		int16_t adc = uv_adc_read(this->adc_chn);
		if (adc < 0) {
			adc = 0;
		}
		int32_t current = (int32_t) adc * this->sense_ampl / 1000;
		this->current = uv_moving_aver_step(&this->moving_avg, current);
	}
	if (this->state == OUTPUT_STATE_ON) {

		if (this->current > this->limit_fault) {
			set_out(this, false);
			uv_output_set_state(this, OUTPUT_STATE_FAULT);
		}
		else if ((this->current > this->limit_max)) {
			set_out(this, false);
			uv_output_set_state(this, OUTPUT_STATE_OVERLOAD);
		}
		else {
			set_out(this, true);
		}
	}
	else {
		set_out(this, false);
	}
}





#endif
