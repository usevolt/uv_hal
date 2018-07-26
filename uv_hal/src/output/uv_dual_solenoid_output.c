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



#include "uv_dual_solenoid_output.h"

#if CONFIG_DUAL_SOLENOID_OUTPUT



void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_pwm_channel_t pwm_a, uv_pwm_channel_t pwm_b,
		uv_adc_channels_e adc_common,
		uint16_t dither_freq, int16_t dither_ampl,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload_a, uint32_t emcy_overload_b,
		uint32_t emcy_fault_a, uint32_t emcy_fault_b) {
	this->value = 0;
	this->current_ma = 0;

	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_A], pwm_a, dither_freq,
			dither_ampl, adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_a, emcy_fault_a);

	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_B], pwm_b, dither_freq,
			dither_ampl, adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_b, emcy_fault_b);
}






void uv_dual_solenoid_output_step(uv_dual_solenoid_output_st *this, uint16_t step_ms) {

	// update current output
	int16_t ca = uv_solenoid_output_get_current(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	int16_t cb = uv_solenoid_output_get_current(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
	this->current_ma = (ca) ? ca : -cb;
}



void uv_dual_solenoid_output_set_conf(uv_dual_solenoid_output_st *this,
					uv_dual_solenoid_output_conf_st *conf) {
	this->conf = *conf;
	uv_solenoid_output_set_conf(&this->solenoid[DUAL_OUTPUT_SOLENOID_A],
			&this->conf.solenoid_conf[DUAL_OUTPUT_SOLENOID_A]);
	uv_solenoid_output_set_conf(&this->solenoid[DUAL_OUTPUT_SOLENOID_B],
			&this->conf.solenoid_conf[DUAL_OUTPUT_SOLENOID_B]);
}



void uv_dual_solenoid_output_set(uv_dual_solenoid_output_st *this, int16_t value_ma) {

}





#endif

