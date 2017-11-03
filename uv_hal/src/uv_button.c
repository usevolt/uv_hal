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
