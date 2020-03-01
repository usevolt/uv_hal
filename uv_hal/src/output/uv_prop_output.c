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



#include "uv_prop_output.h"

#if CONFIG_PROP_OUTPUT


#define PID_P_MAX			20000
#define TARGET_DELAY_MS		20
#define PID_MULTIPLIER		0x10
#define ACC_MIN				10
#define DEC_MIN				60
#define TOGGLE_HYSTERESIS_DEFAULT		100


void uv_prop_output_conf_reset(uv_prop_output_conf_st *this,
		uv_prop_output_limitconf_st *limitconf) {
	uv_solenoid_output_conf_reset(&this->solenoid_conf[0], &limitconf->solenoid_limitconf[0]);
	uv_solenoid_output_conf_reset(&this->solenoid_conf[1], &limitconf->solenoid_limitconf[1]);
	this->acc = CONFIG_PROP_OUTPUT_ACC_DEF;
	this->dec = CONFIG_PROP_OUTPUT_DEC_DEF;
	this->invert = false;
	this->assembly_invert = false;
}



void uv_prop_output_init(uv_prop_output_st *this,
		uv_prop_output_conf_st *conf, uv_prop_output_limitconf_st *limitconf) {
	this->conf = conf;
	this->limitconf = limitconf;
	this->target_req = 0;
	this->last_target_req = 0;
	this->target = 0;
	this->target_mult = 0;
	this->mode = PROP_OUTPUT_MODE_PROP_NORMAL;
	this->state = OUTPUT_STATE_OFF;
	uv_pid_init(&this->target_pid, PID_P_MAX, 0, 0);
	uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

	this->toggle_threshold = PROP_OUTPUT_TOGGLE_THRESHOLD_DEFAULT;
	this->last_hyst = 0;
	this->toggle_on = 0;
	this->toggle_limit_ms_pos = PROP_OUTPUT_TOGGLE_LIMIT_MS_DEFAULT;
	this->toggle_limit_ms_neg = PROP_OUTPUT_TOGGLE_LIMIT_MS_DEFAULT;
	uv_hysteresis_init(&this->toggle_hyst,
			this->toggle_threshold, TOGGLE_HYSTERESIS_DEFAULT, false);
	uv_delay_init(&this->toggle_delay, this->toggle_limit_ms_pos);
	this->enable_delay_ms = PROP_OUTPUT_ENABLE_DELAY_MS_DEFAULT;
	uv_delay_init(&this->enable_delay, this->enable_delay_ms);
}




void uv_prop_output_step(uv_prop_output_st *this, uint16_t step_ms) {

	if (this->state == OUTPUT_STATE_DISABLED ||
			this->state == OUTPUT_STATE_OVERLOAD ||
			this->state == OUTPUT_STATE_FAULT) {
		this->target = 0;
		this->target_req = 0;
		uv_delay_init(&this->enable_delay, this->enable_delay_ms);
	}
	else {
		if (uv_delay(&this->target_delay, step_ms)) {
			uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

			uint16_t acc = this->conf->acc;
			uint16_t dec = this->conf->dec;
			LIMIT_MAX(acc, 100);
			LIMIT_MAX(dec, 100);

			// update hysteresis parameters
			// because of uv_hysteresis module compares greater-than, and not greater-or-equal,
			// maximum value is not valid threshold as that would never trigger the output.
			this->toggle_hyst.trigger_value = (this->toggle_threshold < PROP_VALUE_MAX) ?
					this->toggle_threshold : PROP_VALUE_MAX - 1;


			int32_t target_req = this->target_req;
			LIMITS(target_req, PROP_VALUE_MIN, PROP_VALUE_MAX);

			if (target_req == 0 ||
					(int32_t) target_req * this->last_target_req < 0) {
				this->toggle_hyst.result = 0;
				uv_delay_init(&this->enable_delay, this->enable_delay_ms);
			}
			this->last_target_req = target_req;

			bool hyston = uv_hysteresis_step(&this->toggle_hyst, abs(target_req));

			// enable delay prevents the output to go ON if the enable delay
			// has not been passed
			bool on = (this->mode == PROP_OUTPUT_MODE_PROP_NORMAL) ?
							!!target_req : hyston;
			if (on) {
				uv_delay(&this->enable_delay, TARGET_DELAY_MS);
			}
			else {
				uv_delay_init(&this->enable_delay, this->enable_delay_ms);
			}
			if (!uv_delay_has_ended(&this->enable_delay)) {
				target_req = 0;
				// we affect the hysteresis value only if the toggle state is off.
				// This makes sure that the output is always possible to
				// switch off without any delays.
				if (!this->toggle_on) {
					hyston = false;
				}
				else {
					if (this->mode != PROP_OUTPUT_MODE_PROP_NORMAL) {
						uv_delay_end(&this->enable_delay);
					}
				}
			}


			// calculate the target value depending on the mode
			if (this->mode == PROP_OUTPUT_MODE_PROP_TOGGLE ||
					this->mode == PROP_OUTPUT_MODE_ONOFF_TOGGLE) {
				if (hyston) {
					if (!this->last_hyst) {
						if (!!this->toggle_on) {
							this->toggle_on = 0;
						}
						else {
							if (target_req > 0) {
								this->toggle_on = 1;
							}
							else {
								this->toggle_on = -1;
							}
						}
					}
					uv_delay_init(&this->toggle_delay,
							(this->toggle_on > 0) ?
									this->toggle_limit_ms_pos :
									this->toggle_limit_ms_neg);
				}
				if (((this->toggle_on > 0) && this->toggle_limit_ms_pos) ||
					((this->toggle_on < 0) && this->toggle_limit_ms_neg)) {
					if (uv_delay(&this->toggle_delay, TARGET_DELAY_MS)) {
						this->toggle_on = 0;
					}
				}
				target_req = (this->toggle_on) ?
						((this->toggle_on > 0) ? 1000 : -1000) : 0;
			}
			else {
				this->toggle_on = 0;
				if (this->mode == PROP_OUTPUT_MODE_ONOFF_NORMAL) {
					if (hyston) {
						if (target_req > 0) {
							target_req = PROP_VALUE_MAX;
						}
						else if (target_req < 0) {
							target_req = PROP_VALUE_MIN;
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
					// PROP_OUTPUT_MODE_PROP_NORMAL
					// Nothing to do here
				}
			}
			this->last_hyst = hyston;



			// calculate the output target value depending on the conf parameters
			// when acc and dec are maximum, the pid controller is bypassed
			if (this->mode == PROP_OUTPUT_MODE_ONOFF_NORMAL ||
					this->mode == PROP_OUTPUT_MODE_ONOFF_TOGGLE ||
					(acc == PROP_ACC_MAX &&
							dec == PROP_DEC_MAX)) {
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

		if (this->state == OUTPUT_STATE_ON ||
				this->state == OUTPUT_STATE_OFF) {
			this->state = this->target ? OUTPUT_STATE_ON : OUTPUT_STATE_OFF;
		}
	}
}


void uv_prop_output_enable(uv_prop_output_st *this) {
	if (this->state == OUTPUT_STATE_DISABLED) {
		this->state = OUTPUT_STATE_OFF;
	}
}









#endif

