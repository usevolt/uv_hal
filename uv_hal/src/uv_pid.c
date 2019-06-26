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




#include "uv_pid.h"
#include <uv_can.h>


#if CONFIG_PID


/// @brief: Initializes the PID structure
void uv_pid_init(uv_pid_st *this, uint32_t p, uint32_t i, uint32_t d) {
	this->p = p;
	this->i = i;
	this->d = d;
	this->input = 0;
	this->output = 0;
	this->sum = 0;
	this->max_sum = INT16_MAX;
	this->min_sum = INT16_MIN;
	this->target = 0;
	this->last_err = 0;
	this->state = PID_STATE_ON;
}

/// @brief: PID step function
void uv_pid_step(uv_pid_st *this, uint16_t step_ms, int32_t input) {

	bool on = true;

	if (this->state == PID_STATE_OFF) {
		on = false;
	}
	else if (this->state == PID_STATE_OFF_REQ) {
		input = 0;
		if ((this->output < PID_OFF_REQ_TOLERANCE) &&
				(this->output > -PID_OFF_REQ_TOLERANCE)) {
			this->state = PID_STATE_OFF;
			on = false;
		}
	}
	else {

	}

	if (on) {
		// the default step ms in milliseconds for which the P, I and D factors
		// are scaled for.
		uint8_t def_step_ms = 20;

		// input is updated after d has been calculated
		this->input = input;

		// error value
		int32_t err = (this->target - this->input);
		// error sum
		if ((int32_t) this->sum + err > this->max_sum) {
			this->sum = this->max_sum;
		}
		else if ((int32_t) this->sum + err < this->min_sum) {
			this->sum = this->min_sum;
		}
		else {
			this->sum += err;
		}

		int32_t d = (int32_t) (err - this->last_err) *
				((int32_t) this->d * def_step_ms / (int32_t) step_ms) / 0x1000;
		int32_t p = (int32_t) err * ((int32_t) this->p * def_step_ms / (int32_t) step_ms) / 0x10000;
		int32_t i = (int32_t) this->sum * ((int32_t) this->i * def_step_ms / (int32_t) step_ms) / 0x10000;

		// lastly sum everything up
		this->output = (p + i + d);

		this->last_err = err;

	}
	else {
		uv_pid_reset(this);
	}
}


#endif
