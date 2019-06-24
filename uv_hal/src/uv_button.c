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


#include "uv_button.h"

#if CONFIG_BUTTON


#define this (b)


uv_errors_e uv_button_init(uv_button_st *b, uv_gpios_e gpio, bool invert) {
	this->is_down = false;
	uv_delay_init(CONFIG_BUTTON_LONG_PRESS_TIME_MS, &this->delay);
	this->gpio = gpio;
	this->invert = invert;
	this->pressed = false;
	this->released = false;
	this->long_pressed = false;
	this->is_long_press = false;
	return uv_err(ERR_NONE);
}

uv_errors_e uv_button_step(uv_button_st *b, unsigned int step_ms) {

	this->released = this->pressed = this->long_pressed = false;


	bool val = uv_gpio_get(this->gpio);
	if (this->invert) val = !val;

	if (val && !this->is_down) {
		// button was pressed
		this->pressed = true;
		this->press_count = 0;
	}
	else if (!val && this->is_down) {
		// button was released
		if (!this->is_long_press) {
			this->released = true;
		}
		this->is_long_press = false;
		// reinitialize the delay
		uv_delay_init(CONFIG_BUTTON_LONG_PRESS_TIME_MS, &this->delay);
	}
	else if (val) {
		// button is continuously pressed,
		// just decrease the pressing delay for long presses
		if (uv_delay(step_ms, &this->delay)) {
			if (!this->is_long_press) {
				this->long_pressed = true;
				this->is_long_press = true;
			}
			this->pressed = true;
			this->press_count++;
			int32_t delay = CONFIG_BUTTON_SEQUENTAL_PRESS_TIME_MS;
			delay -= this->press_count * CONFIG_BUTTON_SEQ_PRESS_ACCELERATION;
			if (delay < 1 || delay > CONFIG_BUTTON_SEQUENTAL_PRESS_TIME_MS) {
				delay = 1;
			}
			uv_delay_init(delay, &this->delay);
		}
	}

	this->is_down = val;

	// if button is up, do nothing

	return uv_err(ERR_NONE);
}

#endif
