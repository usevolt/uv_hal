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



#include "uv_dual_solenoid_output.h"

#if CONFIG_DUAL_SOLENOID_OUTPUT



#define OUTPUT_FAULT_FREEZE_MS			10



void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_prop_output_conf_st *conf,
		uv_prop_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_a, uv_pwm_channel_t pwm_b,
		uv_adc_channels_e adc_common,
		uint16_t dither_freq, int16_t dither_ampl,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload_a, uint32_t emcy_overload_b,
		uint32_t emcy_fault_a, uint32_t emcy_fault_b) {

	uv_prop_output_init((uv_prop_output_st*) this, conf, limitconf);

	this->current_ma = 0;
	this->out = 0;
	this->unidir = false;
	this->out_type = SOLENOID_OUTPUT_MODE_CURRENT;

	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_A],
			&conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_A],
			&limitconf->solenoid_limitconf[DUAL_OUTPUT_SOLENOID_A],
			pwm_a, dither_freq, dither_ampl,
			adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_a, emcy_fault_a);

	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_B],
			&conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_B],
			&limitconf->solenoid_limitconf[DUAL_OUTPUT_SOLENOID_B],
			pwm_b, dither_freq, dither_ampl,
			adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_b, emcy_fault_b);
}




void uv_dual_solenoid_output_step(uv_dual_solenoid_output_st *this, uint16_t step_ms) {
	uv_prop_output_step((uv_prop_output_st *) this, step_ms);

	uv_prop_output_conf_st *conf = uv_prop_output_get_conf((uv_prop_output_st*) this);

	uv_dual_solenoid_output_solenoids_e sa = (conf->invert) ?
			DUAL_OUTPUT_SOLENOID_B : DUAL_OUTPUT_SOLENOID_A;
	uv_dual_solenoid_output_solenoids_e sb = (conf->invert) ?
			DUAL_OUTPUT_SOLENOID_A : DUAL_OUTPUT_SOLENOID_B;


	// if other solenoid output modes are defined, DUAL_OUTPUT_SOLENOID_B follows
	// DUAL_OUTPUT_SOLENOID_A's mode
	uv_solenoid_output_set_mode(&this->solenoid[1],
			uv_solenoid_output_get_mode(&this->solenoid[0]));


	// make sure the solenoid outputs are in right mode
	uv_prop_output_modes_e mode = uv_prop_output_get_mode((uv_prop_output_st *) this);
	if (mode == PROP_OUTPUT_MODE_ONOFF_NORMAL ||
			mode == PROP_OUTPUT_MODE_ONOFF_TOGGLE) {
		uv_solenoid_output_set_mode(&this->solenoid[0], SOLENOID_OUTPUT_MODE_ONOFF);
		uv_solenoid_output_set_mode(&this->solenoid[1], SOLENOID_OUTPUT_MODE_ONOFF);
	}
	else if (mode == PROP_OUTPUT_MODE_PROP_NORMAL ||
			mode == PROP_OUTPUT_MODE_PROP_TOGGLE) {
		uv_solenoid_output_set_mode(&this->solenoid[0], this->out_type);
		uv_solenoid_output_set_mode(&this->solenoid[1], this->out_type);
	}
	else {

	}


	int16_t target = uv_prop_output_get_target((uv_prop_output_st *) this);
	if (this->unidir) {
		uv_solenoid_output_set(&this->solenoid[DUAL_OUTPUT_SOLENOID_A], abs(target));
		uv_solenoid_output_set(&this->solenoid[DUAL_OUTPUT_SOLENOID_B], 0);
	}
	else {
		if (target > 0) {
			uv_solenoid_output_set(&this->solenoid[sb], 0);
			uv_solenoid_output_freeze_fault_detection(&this->solenoid[sb],
					OUTPUT_FAULT_FREEZE_MS);

			// only set output active if the other direction has gone to zero
			if (uv_solenoid_output_get_pwm_dc(&this->solenoid[sb]) == 0) {
				uv_solenoid_output_set(&this->solenoid[sa], abs(target));
			}
		}
		else {
			uv_solenoid_output_set(&this->solenoid[sa], 0);
			uv_solenoid_output_freeze_fault_detection(&this->solenoid[sa],
					OUTPUT_FAULT_FREEZE_MS);

			// only set output active if the other direction has gone to zero
			if (uv_solenoid_output_get_pwm_dc(&this->solenoid[sa]) == 0) {
				uv_solenoid_output_set(&this->solenoid[sb], abs(target));
			}
		}
	}

	int16_t maxspeed_scaler = uv_dual_solenoid_output_get_maxspeed_scaler(this);
	LIMITS(maxspeed_scaler, 0, 1000);

	uv_solenoid_output_set_maxspeed_scaler(
			&this->solenoid[DUAL_OUTPUT_SOLENOID_A], maxspeed_scaler);
	uv_solenoid_output_set_maxspeed_scaler(
			&this->solenoid[DUAL_OUTPUT_SOLENOID_B], maxspeed_scaler);

	uv_solenoid_output_step(&this->solenoid[DUAL_OUTPUT_SOLENOID_A], step_ms);
	if (this->unidir) {
		// control solenoid B with the same value as solenoid A
		uv_pwm_set(this->solenoid[DUAL_OUTPUT_SOLENOID_B].pwm_chn,
				uv_solenoid_output_get_pwm_dc(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]));
	}
	else {
		uv_solenoid_output_step(&this->solenoid[DUAL_OUTPUT_SOLENOID_B], step_ms);
	}


	// update current output
	int16_t ca = uv_solenoid_output_get_out(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	int16_t cb = uv_solenoid_output_get_out(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
	this->current_ma = (ca) ? uv_solenoid_output_get_current(&this->solenoid[0]) :
			-uv_solenoid_output_get_current(&this->solenoid[1]);
	this->out = (ca) ? ca : -cb;
	// only assembly invert should affect the direction here
	this->current_ma *= (conf->invert) ? -1 : 1;


}



void uv_dual_solenoid_output_set_conf(uv_dual_solenoid_output_st *this,
					uv_prop_output_conf_st *conf) {
	uv_prop_output_set_conf((uv_prop_output_st*) this, conf);
	uv_solenoid_output_set_conf(&this->solenoid[DUAL_OUTPUT_SOLENOID_A],
			&conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_A]);
	uv_solenoid_output_set_conf(&this->solenoid[DUAL_OUTPUT_SOLENOID_B],
			&conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_B]);
}









#endif

