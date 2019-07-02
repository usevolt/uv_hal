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


#define PID_P_MAX			20000
#define TARGET_DELAY_MS		20
#define PID_MULTIPLIER		0x10
#define ACC_MIN				10
#define DEC_MIN				60
#define TOGGLE_HYSTERESIS_DEFAULT		100


void uv_dual_solenoid_output_conf_reset(uv_dual_solenoid_output_conf_st *this,
		uv_dual_solenoid_output_limitconf_st *limitconf) {
	uv_solenoid_output_conf_reset(&this->solenoid_conf[0], &limitconf->solenoid_limitconf[0]);
	uv_solenoid_output_conf_reset(&this->solenoid_conf[1], &limitconf->solenoid_limitconf[1]);
	this->acc = CONFIG_DUAL_SOLENOID_ACC_DEF;
	this->dec = CONFIG_DUAL_SOLENOID_DEC_DEF;
	this->invert = false;
	this->assembly_invert = false;
}



void uv_dual_solenoid_output_init(uv_dual_solenoid_output_st *this,
		uv_dual_solenoid_output_conf_st *conf,
		uv_dual_solenoid_output_limitconf_st *limitconf,
		uv_pwm_channel_t pwm_a, uv_pwm_channel_t pwm_b,
		uv_adc_channels_e adc_common,
		uint16_t dither_freq, int16_t dither_ampl,
		uint16_t sense_ampl, uint16_t max_current, uint16_t fault_current,
		uint32_t emcy_overload_a, uint32_t emcy_overload_b,
		uint32_t emcy_fault_a, uint32_t emcy_fault_b) {
	this->conf = conf;
	this->limitconf = limitconf;
	this->target_req = 0;
	this->target = 0;
	this->target_mult = 0;
	this->current_ma = 0;
	this->out = 0;
	this->mode = DUAL_SOLENOID_OUTPUT_MODE_PROP_NORMAL;
	uv_pid_init(&this->target_pid, PID_P_MAX, 0, 0);
	uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

	this->toggle_threshold = DUAL_SOLENOID_OUTPUT_TOGGLE_THRESHOLD_DEFAULT;
	this->last_hyst = 0;
	this->toggle_on = 0;
	this->toggle_limit_ms = DUAL_SOLENOID_OUTPUT_TOGGLE_LIMIT_MS_DEFAULT;
	uv_hysteresis_init(&this->toggle_hyst,
			this->toggle_threshold, TOGGLE_HYSTERESIS_DEFAULT, false);
	uv_delay_init(&this->toggle_delay, this->toggle_limit_ms);


	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_A],
			&this->conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_A],
			&this->limitconf->solenoid_limitconf[DUAL_OUTPUT_SOLENOID_A],
			pwm_a, dither_freq, dither_ampl,
			adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_a, emcy_fault_a);

	uv_solenoid_output_init(&this->solenoid[DUAL_OUTPUT_SOLENOID_B],
			&this->conf->solenoid_conf[DUAL_OUTPUT_SOLENOID_B],
			&this->limitconf->solenoid_limitconf[DUAL_OUTPUT_SOLENOID_B],
			pwm_b, dither_freq, dither_ampl,
			adc_common, sense_ampl, max_current, fault_current,
			emcy_overload_b, emcy_fault_b);
}






void uv_dual_solenoid_output_step(uv_dual_solenoid_output_st *this, uint16_t step_ms) {


	uv_dual_solenoid_output_solenoids_e sa = (this->conf->assembly_invert) ?
			DUAL_OUTPUT_SOLENOID_B : DUAL_OUTPUT_SOLENOID_A;
	uv_dual_solenoid_output_solenoids_e sb = (this->conf->assembly_invert) ?
			DUAL_OUTPUT_SOLENOID_A : DUAL_OUTPUT_SOLENOID_B;


	// if other solenoid output modes are defined, DUAL_OUTPUT_SOLENOID_B follows
	// DUAL_OUTPUT_SOLENOID_A's mode
	uv_solenoid_output_set_mode(&this->solenoid[1],
			uv_solenoid_output_get_mode(&this->solenoid[0]));


	if (uv_delay(&this->target_delay, step_ms)) {
		uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

		uint16_t acc = this->conf->acc;
		uint16_t dec = this->conf->dec;
		LIMIT_MAX(acc, 100);
		LIMIT_MAX(dec, 100);

		// make sure the solenoid ouptuts are in right mode
		if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_NORMAL ||
				this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_TOGGLE) {
			uv_solenoid_output_set_mode(&this->solenoid[0], SOLENOID_OUTPUT_MODE_ONOFF);
			uv_solenoid_output_set_mode(&this->solenoid[1], SOLENOID_OUTPUT_MODE_ONOFF);
		}
		else if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_PROP_NORMAL ||
				this->mode == DUAL_SOLENOID_OUTPUT_MODE_PROP_TOGGLE) {
			uv_solenoid_output_set_mode(&this->solenoid[0], SOLENOID_OUTPUT_MODE_CURRENT);
			uv_solenoid_output_set_mode(&this->solenoid[1], SOLENOID_OUTPUT_MODE_CURRENT);
		}
		else {

		}
		// update hysteresis parameters
		// because of uv_hysteresis module compares greater-than, and not greater-or-equal,
		// maximum value is not valid threshold as that would never trigger the output.
		this->toggle_hyst.trigger_value = (this->toggle_threshold < DUAL_SOLENOID_VALUE_MAX) ?
				this->toggle_threshold : DUAL_SOLENOID_VALUE_MAX - 1;


		int32_t target_req = this->target_req;
		LIMITS(target_req, DUAL_SOLENOID_VALUE_MIN, DUAL_SOLENOID_VALUE_MAX);

		if (target_req == 0) {
			this->toggle_hyst.result = 0;
		}

		uv_hysteresis_step(&this->toggle_hyst, abs(target_req));
		// calculate the target value depending on the mode
		if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_PROP_TOGGLE ||
				this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_TOGGLE) {
			if (uv_hysteresis_get_output(&this->toggle_hyst)) {
				if (!this->last_hyst) {
					if (target_req > 0) {
						if (this->toggle_on == -1) {
							this->toggle_on = 1;
						}
						else {
							this->toggle_on = (this->toggle_on) ? 0 : 1;
						}
					}
					else {
						if (this->toggle_on == 1) {
							this->toggle_on = -1;
						}
						else {
							this->toggle_on = (this->toggle_on) ? 0 : -1;
						}
					}
				}
				uv_delay_init(&this->toggle_delay, this->toggle_limit_ms);
			}
			if (this->toggle_limit_ms && uv_delay(&this->toggle_delay, TARGET_DELAY_MS)) {
				this->toggle_on = 0;
			}
			target_req = (this->toggle_on) ? ((this->toggle_on > 0) ? 1000 : -1000) : 0;
		}
		else if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_NORMAL) {
			if (uv_hysteresis_get_output(&this->toggle_hyst)) {
				if (target_req > 0) {
					target_req = DUAL_SOLENOID_VALUE_MAX;
				}
				else if (target_req < 0) {
					target_req = DUAL_SOLENOID_VALUE_MIN;
				}
				else {
					target_req = 0;
				}
			}
			else {
				target_req = 0;
			}
		}
		else {
			// DUAL_SOLENOID_OUTPUT_MODE_PROP_NORMAL
			// Nothing to do here
		}
		this->last_hyst = uv_hysteresis_get_output(&this->toggle_hyst);



		// calculate the output target value depending on the conf parameters
		// when acc and dec are maximum, the pid controller is bypassed
		if (this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_NORMAL ||
				this->mode == DUAL_SOLENOID_OUTPUT_MODE_ONOFF_TOGGLE ||
				(acc == DUAL_SOLENOID_ACC_MAX &&
						dec == DUAL_SOLENOID_DEC_MAX)) {
			this->target = target_req;
		}
		else {
			// different moving average values for accelerating and decelerating
			// maximum decelerating time is 1 sec
			if ((abs(target_req) > abs(this->target)) ||
					((int32_t) target_req * this->target < 0)) {
				// accelerating
				uv_pid_set_p(&this->target_pid, (uint32_t) PID_P_MAX * acc * acc / 10000);
			}
			else {
				// decelerating
				uv_pid_set_p(&this->target_pid, (uint32_t) PID_P_MAX * dec * dec / 10000);
			}

			uv_pid_set_target(&this->target_pid, target_req * PID_MULTIPLIER);
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
				this->target = target_req;
				this->target_mult = target_req * PID_MULTIPLIER;
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
	int16_t ca = uv_solenoid_output_get_out(&this->solenoid[DUAL_OUTPUT_SOLENOID_A]);
	int16_t cb = uv_solenoid_output_get_out(&this->solenoid[DUAL_OUTPUT_SOLENOID_B]);
	this->current_ma = (ca) ? uv_solenoid_output_get_current(&this->solenoid[0]) :
			-uv_solenoid_output_get_current(&this->solenoid[1]);
	this->out = (ca) ? ca : -cb;
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

