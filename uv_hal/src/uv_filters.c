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

#include "uv_filters.h"


void uv_moving_aver_init (uv_moving_aver_st *avr, int32_t cnt) {
	avr->sum = 0;
	avr->count = cnt;
	avr->cur_count = 0;
	avr->val = 0;
}

void uv_moving_aver_reset (uv_moving_aver_st *avr) {
	avr->sum = 0;
	avr->cur_count = 0;
	avr->val = 0;
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

void uv_moving_aver_set_count(uv_moving_aver_st *this, int32_t value) {
	if (!value) {
		value = 1;
	}
	this->count = value;
}



void uv_hysteresis_init(uv_hysteresis_st *this, int32_t trigger_value,
		int32_t hysteresis, bool invert) {
	this->trigger_value = trigger_value;
	this->invert = invert;
	this->hysteresis = hysteresis;
	this->result = this->invert;
}

bool uv_hysteresis_step(uv_hysteresis_st *this, int32_t value) {
	if (this->invert) {
		if ((!this->result) && (value < this->trigger_value)) {
			this->result = 1;
		}
		else if ((this->result) && (value > this->trigger_value + this->hysteresis)) {
			this->result = 0;
		}
		else {

		}
	}
	else {
		if ((!this->result) && (value > this->trigger_value)) {
			this->result = 1;
		}
		else if ((this->result) && (value < this->trigger_value - this->hysteresis)) {
			this->result = 0;
		}
		else {

		}
	}

	return (bool) this->result;
}
