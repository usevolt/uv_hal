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


#define PID_P_MAX			20000
#define TARGET_DELAY_MS		20
#define PID_MULTIPLIER		0x10
#define ACC_MIN				10
#define DEC_MIN				60


void uv_dual_solenoid_output_conf_reset(uv_dual_solenoid_output_conf_st *this) {
	uv_solenoid_output_conf_reset(&this->solenoid_conf[0]);
	uv_solenoid_output_conf_reset(&this->solenoid_conf[1]);
	this->acc = CONFIG_DUAL_SOLENOID_ACC_DEF;
	this->dec = CONFIG_DUAL_SOLENOID_DEC_DEF;
	this->invert = false;
	this->assembly_invert = false;
}



void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_dual_solenoid_output_conf_st *conf,
		uv_pwm_channel_t pwm_a, uv_pwm_channel_t pwm_b,
		uv_adc_channels_e adc_common,
		uint16_t dither_freq, int16_t dither_ampl,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload_a, uint32_t emcy_overload_b,
		uint32_t emcy_fault_a, uint32_t emcy_fault_b) {
	this->target_req = 0;
	this->target = 0;
	this->target_mult = 0;
	this->current_ma = 0;
	this->out = 0;
	this->conf = conf;
	uv_pid_init(&this->target_pid, PID_P_MAX, 0, 0);
	uv_delay_init(&this->target_delay, TARGET_DELAY_MS);


	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_A],
			&this->conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_A], pwm_a, dither_freq,
			dither_ampl, adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_a, emcy_fault_a);

	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_B],
			&this->conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_B], pwm_b, dither_freq,
			dither_ampl, adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_b, emcy_fault_b);
}






void uv_dual_solenoid_output_step(uv_dual_solenoid_output_st *this, uint16_t step_ms) {


	uv_dual_solenoid_output_solenoids_e sa = (this->conf->assembly_invert) ?
			DUAL_OUTPUT_SOLENOID_B : DUAL_OUTPUT_SOLENOID_A;
	uv_dual_solenoid_output_solenoids_e sb = (this->conf->assembly_invert) ?
			DUAL_OUTPUT_SOLENOID_A : DUAL_OUTPUT_SOLENOID_B;


#if CONFIG_SOLENOID_MODE_ONOFF || CONFIG_SOLENOID_MODE_PWM
	// if other solenoid output modes are defined, DUAL_OUTPUT_SOLENOID_B follows
	// DUAL_OUTPUT_SOLENOID_A's mode
	uv_solenoid_output_set_mode(&this->solenoid[1],
			uv_solenoid_output_get_mode(&this->solenoid[0]));
	#endif


	if (uv_delay(&this->target_delay, step_ms)) {
		uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

		if (this->conf->acc > 100) {
			this->conf->acc = 100;
		}
		if (this->conf->dec > 100) {
			this->conf->dec = 100;
		}
		uint16_t acc = this->conf->acc;
		uint16_t dec = this->conf->dec;

		if (this->target_req > DUAL_SOLENOID_VALUE_MAX) {
			this->target_req = DUAL_SOLENOID_VALUE_MAX;
		}
		else if (this->target_req < DUAL_SOLENOID_VALUE_MIN) {
			this->target_req = DUAL_SOLENOID_VALUE_MIN;
		}
		else {

		}

		// in ONOFF mode acc and dec are always maximum
		if (uv_solenoid_output_get_mode(&this->solenoid[0]) == SOLENOID_OUTPUT_MODE_ONOFF) {
			this->target = this->target_req;
		}
		else {
			// different moving average values for accelerating and decelerating
			// maximum decelerating time is 1 sec
			if ((abs(this->target_req) > abs(this->target)) &&
					((int32_t) this->target_req * this->target >= 0)) {
				// accelerating
				uv_pid_set_p(&this->target_pid, (uint32_t) PID_P_MAX * acc * acc / 10000);
			}
			else {
				// decelerating
				uv_pid_set_p(&this->target_pid, (uint32_t) PID_P_MAX * dec * dec / 10000);
			}

			uv_pid_set_target(&this->target_pid, this->target_req * PID_MULTIPLIER);
			uv_pid_step(&this->target_pid, TARGET_DELAY_MS, this->target_mult);
			this->target_mult += uv_pid_get_output(&this->target_pid);
			if ((this->target_mult / PID_MULTIPLIER) > 1000) {
				this->target_mult = 1000;
			}
			else if ((this->target_mult / PID_MULTIPLIER) < -1000) {
				this->target_mult = -1000;
			}
			else {

			}
			this->target = this->target_mult / PID_MULTIPLIER;

			// clamp output to target value when we're close enough
			if (uv_pid_get_output(&this->target_pid) == 0) {
				this->target = this->target_req;
				this->target_mult = this->target_req * PID_MULTIPLIER;
			}
		}
	}


	if (this->target > 0) {
		uv_solenoid_output_set(&this->solenoid[sb], 0);
		// only set output active if the other direction has gone to zero
		if (uv_solenoid_output_get_pwm_dc(&this->solenoid[sb]) == 0) {
			uv_solenoid_output_set(&this->solenoid[sa], abs(this->target));
		}
	}
	else {
		uv_solenoid_output_set(&this->solenoid[sa], 0);
		// only set output active if the other direction has gone to zero
		if (uv_solenoid_output_get_pwm_dc(&this->solenoid[sa]) == 0) {
			uv_solenoid_output_set(&this->solenoid[sb], abs(this->target));
		}
	}


	uv_solenoid_output_step(&this->solenoid[DUAL_OUTPUT_SOLENOID_A], step_ms);
	uv_solenoid_output_step(&this->solenoid[DUAL_OUTPUT_SOLENOID_B], step_ms);


	// update current output
	int16_t ca = uv_solenoid_output_get_current(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	int16_t cb = uv_solenoid_output_get_current(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
	this->current_ma = (ca) ? ca : -cb;
	this->out = (ca) ? uv_solenoid_output_get_out(&this->solenoid[0]) :
						-uv_solenoid_output_get_out(&this->solenoid[1]);
	// only assembly invert should affect the direction here
	this->current_ma *= (this->conf->assembly_invert) ? -1 : 1;

}



void uv_dual_solenoid_output_set_conf(uv_dual_solenoid_output_st *this,
					uv_dual_solenoid_output_conf_st *conf) {
	this->conf = conf;
	uv_solenoid_output_set_conf(&this->solenoid[DUAL_OUTPUT_SOLENOID_A],
			&this->conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_A]);
	uv_solenoid_output_set_conf(&this->solenoid[DUAL_OUTPUT_SOLENOID_B],
			&this->conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_B]);
}






#endif

