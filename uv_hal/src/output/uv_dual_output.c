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


#include "uv_dual_output.h"

#if CONFIG_DUAL_OUTPUT




void uv_dual_output_init(uv_dual_output_st *this, uv_gpios_e gpio_pos, uv_gpios_e gpio_neg,
		uv_adc_channels_e adc_chn, uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint16_t moving_avg_count, uint32_t emcy_overload, uint32_t emcy_fault) {

	uv_output_init((uv_output_st*) this, adc_chn, 0, sense_ampl, max_current, fault_current,
			moving_avg_count, emcy_overload, emcy_fault);

	this->dir = DUAL_OUTPUT_POS;
	this->gpio_pos = gpio_pos;
	this->gpio_neg = gpio_neg;
	this->invert = false;
	uv_gpio_init_output(this->gpio_pos, false);
	uv_gpio_init_output(this->gpio_neg, false);
}


void uv_dual_output_step(uv_dual_output_st *this, uint16_t step_ms) {
	uv_output_step((uv_output_st*) this, step_ms);


	this->current = uv_output_get_current((uv_output_st *) this);
	this->current *= (((this->dir == DUAL_OUTPUT_NEG) != (this->invert)) ? -1 : 1);

	uv_output_state_e state = uv_dual_output_get_state(this);
	if (state == OUTPUT_STATE_ON) {
		uv_dual_output_set_dir(this, this->dir);
	}
	else {
		uv_dual_output_set_dir(this, DUAL_OUTPUT_OFF);
	}
}



void uv_dual_output_set_dir(uv_dual_output_st *this, uv_dual_output_dir_e value) {
	this->dir = value;
	if (value == DUAL_OUTPUT_OFF) {
		uv_dual_output_set_state(this, OUTPUT_STATE_OFF);
		uv_gpio_set(this->gpio_neg, false);
		uv_gpio_set(this->gpio_pos, false);
	}
	else {
		uv_dual_output_set_state(this, OUTPUT_STATE_ON);
	}
	if (uv_dual_output_get_state(this) == OUTPUT_STATE_ON) {
		uv_gpio_set((this->invert) ? this->gpio_neg : this->gpio_pos,
				(value == DUAL_OUTPUT_POS) ? true : false);
		uv_gpio_set((this->invert) ? this->gpio_pos : this->gpio_neg,
				(value == DUAL_OUTPUT_POS) ? false : true);
	}
}


#endif
