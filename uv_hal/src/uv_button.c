/*
 * uv_button.c
 *
 *  Created on: Aug 11, 2016
 *      Author: usevolt
 */


#include "uv_button.h"

#define this (b)


uv_errors_e uv_button_init(uv_button_st *b, uv_gpios_e gpio, bool invert) {
	this->is_down = false;
	uv_delay_init(BUTTON_LONG_PRESS_TIME_MS, &this->delay);
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
	}
	else if (!val && this->is_down) {
		// button was released
		if (!this->is_long_press) {
			this->released = true;
		}
		this->is_long_press = false;
		// reinitialize the delay
		uv_delay_init(BUTTON_LONG_PRESS_TIME_MS, &this->delay);
	}
	else if (val) {
		// button is continuously pressed,
		// just decrease the pressing delay for long presses
		if (uv_delay(step_ms, &this->delay)) {
			this->long_pressed = true;
			this->is_long_press = true;
		}
	}

	this->is_down = val;

	// if button is up, do nothing

	return uv_err(ERR_NONE);
}

