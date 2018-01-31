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


#include "uv_gpio.h"

#include <chip.h>
#include <pinint_15xx.h>
#include <inmux_15xx.h>
#include <stdio.h>
#include "uv_utilities.h"



// callback function
static void (*callback)(uv_gpios_e) = 0;
static uint8_t int_count = 0;
static uv_gpios_e int_pins[8] = {};




void uv_gpio_add_interrupt_callback(void (*callback_function)(uv_gpios_e)) {
	callback = callback_function;
	Chip_PININT_Init(LPC_GPIO_PIN_INT);
}



uint8_t uv_gpio_get_port(uv_gpios_e gpio) {
	return ((gpio - 1) / 32);
}

uint8_t uv_gpio_get_pin(uv_gpios_e gpio) {
	return ((gpio - 1) % 32);
}



void uv_gpio_init_int(uv_gpios_e gpio, uv_gpio_interrupt_config_e confs) {
	if (int_count < 8) {
		NVIC_EnableIRQ(PIN_INT0_IRQn + int_count);

		Chip_INMUX_PinIntSel(int_count, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));

		Chip_PININT_SetPinModeEdge(LPC_GPIO_PIN_INT, PININTCH(int_count));
		if (confs & INT_RISING_EDGE) {
			Chip_PININT_EnableIntHigh(LPC_GPIO_PIN_INT, PININTCH(int_count));
		}
		if (confs & INT_FALLING_EDGE) {
			Chip_PININT_EnableIntLow(LPC_GPIO_PIN_INT, PININTCH(int_count));
		}
		int_pins[int_count] = gpio;
		int_count++;
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



