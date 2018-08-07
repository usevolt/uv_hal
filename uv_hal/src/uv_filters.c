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

#include "uv_filters.h"


void uv_moving_aver_init (uv_moving_aver_st *avr, int32_t cnt) {
	avr->sum = 0;
	avr->count = cnt;
	avr->cur_count = 0;
}

void uv_moving_aver_reset (uv_moving_aver_st *avr) {
	avr->sum = 0;
	avr->cur_count = 0;
}

int32_t uv_moving_aver_step (uv_moving_aver_st *avr, int32_t val) {
	avr->sum += (val * 0x100);
	avr->cur_count += 1;
	avr->val = avr->sum / avr->cur_count;

	while (avr->cur_count > avr->count) {
		avr->sum -= avr->val;
		avr->cur_count -= 1;
	}

	return (avr->val / 0x100);
}



void uv_hysteresis_init(uv_hysteresis_st *this, int32_t trigger_value,
		int32_t hysteresis, bool invert) {
	this->trigger_value = trigger_value;
	this->invert = invert;
	this->hysteresis = hysteresis;
	this->result = this->invert;
}

bool uv_hysteresis_step(uv_hysteresis_st *this, int32_t value) {
	if ((this->result == this->invert) && (value > this->trigger_value + this->hysteresis)) {
		this->result = !this->invert;
	}
	else if ((this->result != this->invert) && (value < this->trigger_value - this->hysteresis)) {
		this->result = this->invert;
	}
	else {

	}

	return (bool) this->result;
}
