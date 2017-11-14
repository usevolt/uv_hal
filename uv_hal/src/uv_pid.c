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


#if CONFIG_PID


/// @brief: Initializes the PID structure
void uv_pid_init(uv_pid_st *this, uint8_t p, uint8_t i, uint8_t d) {
	this->p = p;
	this->i = i;
	this->d = d;
	this->input = 0;
	this->output = 0;
	this->sum = 0;
}

/// @brief: PID step function
void uv_pid_step(uv_pid_st *this, uint16_t step_ms, int16_t input) {

	// d has to be summed beforehand to get the derivation
	int32_t d = (int32_t) (this->input - input) * this->d / 255;

	// input is updated after d has been calculated
	this->input = input;

	// error value
	int32_t err = this->input - this->output;
	// error sum
	this->sum += err;

	int32_t p = (uint32_t) err * this->p / 255;
	int32_t i = this->sum * this->i / 255;

	// lastly sum everything up
	this->output += p + i + d;
}


#endif
