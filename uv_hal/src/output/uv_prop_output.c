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
#define TOGGLE_HYSTERESIS_DEFAULT		10


void uv_prop_output_conf_reset(uv_prop_output_conf_st *this,
		uv_prop_output_limitconf_st *limitconf) {
	uv_solenoid_output_conf_reset(&this->solenoid_conf[0], limitconf);
	uv_solenoid_output_conf_reset(&this->solenoid_conf[1], limitconf);
	this->acc = CONFIG_PROP_OUTPUT_ACC_DEF;
	this->dec = CONFIG_PROP_OUTPUT_DEC_DEF;
}



void uv_prop_output_init(uv_prop_output_st *this,
		uv_prop_output_conf_st *conf, uv_prop_output_limitconf_st *limitconf) {
	this->conf = conf;
	this->limitconf = limitconf;
	this->target_req = 0;
	this->last_target_req = 0;
	this->target = 0;
	this->target_mult = 0;
	this->mode = PROP_OUTPUT_MODE_PROP;
	this->state = OUTPUT_STATE_OFF;
	uv_pid_init(&this->target_pid, PID_P_MAX, 0, 0);
	uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

	this->enable_pre_delay_ms = 0;
	this->enable_post_delay_ms = 0;
	this->pre_enable_dir = 0;
	this->post_enable_val = 0;
	uv_delay_init(&this->pre_enable_delay, this->enable_pre_delay_ms);
	uv_delay_end(&this->pre_enable_delay);
	uv_delay_init(&this->post_enable_delay, this->enable_post_delay_ms);
	uv_delay_end(&this->post_enable_delay);
	this->maxspeed_scaler = 1000;
	uv_prop_output_clear(this);
}


void uv_prop_output_clear(uv_prop_output_st *this) {
}



void uv_prop_output_step(uv_prop_output_st *this, uint16_t step_ms) {

	if (this->state == OUTPUT_STATE_DISABLED ||
			this->state == OUTPUT_STATE_OPENLOOP ||
			this->state == OUTPUT_STATE_FAULT) {
		this->target = 0;
		this->target_req = 0;
	}
	else {
		if (uv_delay(&this->target_delay, step_ms)) {
			uv_delay_init(&this->target_delay, TARGET_DELAY_MS);

			uint16_t acc = this->conf->acc;
			uint16_t dec = this->conf->dec;
			LIMITS(acc, PROP_ACC_MIN, PROP_ACC_MAX);
			LIMITS(dec, PROP_DEC_MIN, PROP_DEC_MAX);
			LIMITS(this->maxspeed_scaler, 0, 1000);
			LIMITS(this->target, PROP_OUTPUT_TARGET_MIN, PROP_OUTPUT_TARGET_MAX);


			int32_t target_req = this->target_req;
			LIMITS(target_req, PROP_OUTPUT_TARGET_MIN, PROP_OUTPUT_TARGET_MAX);

			// apply the maxspeed scaler to the target value
			target_req = uv_lerpi(this->maxspeed_scaler, 0, target_req);

			if ((int32_t) target_req * this->last_target_req < 0) {
				uv_delay_init(&this->pre_enable_delay, this->enable_pre_delay_ms);
			}

			bool on = (!!target_req);
			bool last_on = (!!this->last_target_req);

			if (on && !last_on) {
				uv_delay_init(&this->pre_enable_delay, this->enable_pre_delay_ms);
				this->pre_enable_dir = (target_req < 0) ?
						-1 : 1;
			}
			else if (!on && last_on) {
				uv_delay_init(&this->post_enable_delay, this->enable_post_delay_ms);
				this->post_enable_val = this->last_target_req;
			}
			else {

			}
			// update the last_target_req variable to hold the current target_req value
			this->last_target_req = target_req;




			// post enable delay delays the turn OFF
			// Since pre enable delay is higher priority, post enable is handled first
			if (uv_delay(&this->post_enable_delay, TARGET_DELAY_MS)) {
			}
			else if (!uv_delay_has_ended(&this->post_enable_delay)) {
				if (target_req * this->post_enable_val < 0) {
					// target_req drives in opposite direction,
					// clear post delay
					uv_delay_end(&this->post_enable_delay);
				}
				else {
					target_req = this->post_enable_val;
				}
			}
			else {

			}

			// pre enable delay delays the turn ON
			uv_delay(&this->pre_enable_delay, TARGET_DELAY_MS);

			if (!uv_delay_has_ended(&this->pre_enable_delay)) {
				// delay the turn on ONLY if toggle mode is not ON.
				// This way toggle modes turn the output off always instantly
				target_req = 0;
				// set post enable val to zero. This prevent output to turn off if
				// the input is clicked very shortly
				this->post_enable_val = 0;
			}
			else {

			}


			// calculate the target value depending on the mode
			if (this->mode == PROP_OUTPUT_MODE_ONOFF) {
				if (target_req > 0) {
					target_req = PROP_OUTPUT_TARGET_MAX;
				}
				else if (target_req < 0) {
					target_req = PROP_OUTPUT_TARGET_MIN;
				}
				else {
					target_req = 0;
				}
			}
			else {
				// PROP_OUTPUT_MODE_PROP_NORMAL
				// Nothing to do here
			}



			// calculate the output target value depending on the conf parameters
			// when acc and dec are maximum, the pid controller is bypassed
			if (this->mode == PROP_OUTPUT_MODE_ONOFF ||
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
					uv_pid_set_p(&this->target_pid,
							(uint32_t) PID_P_MAX * acc * acc / (PROP_ACC_MAX * PROP_ACC_MAX));
				}
				else {
					// decelerating
					uv_pid_set_p(&this->target_pid,
							(uint32_t) PID_P_MAX * dec * dec / (PROP_DEC_MAX * PROP_DEC_MAX));
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
				if (abs(uv_pid_get_output(&this->target_pid)) <= 2) {
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

