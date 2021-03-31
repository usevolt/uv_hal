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

#include <chip.h>
#include <pinint_15xx.h>
#include <inmux_15xx.h>
#include <stdio.h>
#include "uv_utilities.h"



// callback function
static void (*callback)(uv_gpios_e) = 0;
#define INT_COUNT	8
static uv_gpios_e int_pins[INT_COUNT] = { };





void uv_gpio_interrupt_init(void (*callback_function)(uv_gpios_e)) {
	memset(int_pins, 0, sizeof(int_pins));
	callback = callback_function;
	Chip_PININT_Init(LPC_GPIO_PIN_INT);
}



uint8_t uv_gpio_get_port(uv_gpios_e gpio) {
	return ((gpio - 1) / 32);
}

uint8_t uv_gpio_get_pin(uv_gpios_e gpio) {
	return ((gpio - 1) % 32);
}



uv_errors_e uv_gpio_enable_int(uv_gpios_e gpio, uv_gpio_interrupt_config_e confs) {
	uv_errors_e ret = ERR_HARDWARE_NOT_SUPPORTED;
	uint8_t index = INT_COUNT;
	for (uint8_t i = 0; i < INT_COUNT; i++) {
		if (int_pins[i] == gpio) {
			ret = ERR_NO_NEW_VALUES;
			break;
		}
		else if (int_pins[i] == 0) {
			index = i;
			ret = ERR_NONE;
		}
		else {

		}
	}
	if (ret == ERR_NONE) {
		NVIC_SetPriority(PIN_INT0_IRQn + index, 1);

		Chip_INMUX_PinIntSel(index, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));

		Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(index));
		if (confs & INT_RISING_EDGE) {
			Chip_PININT_EnableIntHigh(LPC_GPIO_PIN_INT, PININTCH(index));
		}
		if (confs & INT_FALLING_EDGE) {
			Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(index));
		}
		int_pins[index] = gpio;
		NVIC_EnableIRQ(PIN_INT0_IRQn + index);
	}

	return ret;
}

void uv_gpio_disable_int(uv_gpios_e gpio) {
	uint8_t index = INT_COUNT;
	for (uint8_t i = 0; i < INT_COUNT; i++) {
		if (int_pins[i] == gpio) {
			index = i;
			break;
		}
	}
	if (index < INT_COUNT) {
		// disable the interrupt on the given pin
		NVIC_DisableIRQ(PIN_INT0_IRQn + index);
		int_pins[index] = 0;
	}
}



void PIN_INT0_IRQHandler(void) {
	if (callback) {
		callback(int_pins[0]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH0);
}

void PIN_INT1_IRQHandler(void) {
	if (callback) {
		callback(int_pins[1]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH1);
}

void PIN_INT2_IRQHandler(void) {
	if (callback) {
		callback(int_pins[2]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH2);
}

void PIN_INT3_IRQHandler(void) {
	if (callback) {
		callback(int_pins[3]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH3);
}

void PIN_INT4_IRQHandler(void) {
	if (callback) {
		callback(int_pins[4]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH4);
}

void PIN_INT5_IRQHandler(void) {
	if (callback) {
		callback(int_pins[5]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH5);
}

void PIN_INT6_IRQHandler(void) {
	if (callback) {
		callback(int_pins[6]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH6);
}

void PIN_INT7_IRQHandler(void) {
	if (callback) {
		callback(int_pins[7]);
	}
	Chip_PININT_ClearIntStatus(LPC_GPIO_PIN_INT, PININTCH7);
}



