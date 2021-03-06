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


#include "uv_gpio.h"


#include <stdio.h>
#include "uv_utilities.h"



// callback function
static void (*callback)(uv_gpios_e) = 0;



uv_errors_e uv_gpio_enable_int(uv_gpios_e gpio, uv_gpio_interrupt_config_e confs) {
	return ERR_NONE;
}

/// @brief: Disables interrupts on pin **gpio**
void uv_gpio_disable_int(uv_gpios_e gpio) {

}



void uv_gpio_interrupt_init(void (*callback_function)(uv_gpios_e)) {

}


void uv_gpio_add_interrupt_callback(void (*callback_function)(uv_gpios_e)) {

	callback = callback_function;
}

uint8_t uv_gpio_get_port(uv_gpios_e gpio) {
	return ((gpio - 1) / 32);
}

uint8_t uv_gpio_get_pin(uv_gpios_e gpio) {
	return ((gpio - 1) % 32);
}




