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

		// d has to be summed beforehand to get the derivation
		int32_t d = (int32_t) (this->input - input) * (this->d * def_step_ms / step_ms) / 0x100;

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

		int32_t p = (int32_t) err * (this->p * def_step_ms / step_ms) / 0x10000;
		int32_t i = (int32_t) this->sum * (this->i * def_step_ms / step_ms) / 0x10000;

		// lastly sum everything up
		this->output = p + i + d;

	}
	else {
		uv_pid_reset(this);
	}
}


#endif
