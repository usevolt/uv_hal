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
#include "chip.h"
#include <chip.h>
#include <stdio.h>
#include "uv_utilities.h"




// callback function
static void (*callback)(uv_gpios_e) = 0;





void uv_gpio_interrupt_init(void (*callback_function)(uv_gpios_e)) {
	callback = callback_function;
	Chip_GPIOINT_Init(LPC_GPIOINT);
}



uint8_t uv_gpio_get_port(uv_gpios_e gpio) {
	return ((gpio - 1) / 32);
}

uint8_t uv_gpio_get_pin(uv_gpios_e gpio) {
	return ((gpio - 1) % 32);
}



uv_errors_e uv_gpio_enable_int(uv_gpios_e gpio, uv_gpio_interrupt_config_e confs) {
	uv_errors_e ret = ERR_NONE;
	if (uv_gpio_get_port(gpio) == 0 || uv_gpio_get_port(gpio) == 2) {
		NVIC_SetPriority(GPIO_IRQn, 1);

		uv_gpio_disable_int(gpio);

		Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT,
				uv_gpio_get_port(gpio),
				(1 << uv_gpio_get_pin(gpio)));

		if ((confs & INT_RISING_EDGE) ||
				(confs & INT_LEVEL_HIGH)) {
			Chip_GPIOINT_SetIntRising(LPC_GPIOINT, uv_gpio_get_port(gpio),
					Chip_GPIOINT_GetIntRising(LPC_GPIOINT,
							uv_gpio_get_port(gpio)) | (1 << uv_gpio_get_pin(gpio)));
		}
		if ((confs & INT_FALLING_EDGE) ||
				(confs & INT_LEVEL_LOW)) {
			Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, uv_gpio_get_port(gpio),
					Chip_GPIOINT_GetIntFalling(LPC_GPIOINT,
							uv_gpio_get_port(gpio)) | (1 << uv_gpio_get_pin(gpio)));
		}
		NVIC_EnableIRQ(GPIO_IRQn);
	}
	else {
		ret = ERR_HARDWARE_NOT_SUPPORTED;
	}

	return ret;
}

void uv_gpio_disable_int(uv_gpios_e gpio) {
	if (uv_gpio_get_port(gpio) == 0 || uv_gpio_get_port(gpio) == 2) {
		Chip_GPIOINT_SetIntRising(LPC_GPIOINT, uv_gpio_get_port(gpio),
				Chip_GPIOINT_GetIntRising(LPC_GPIOINT,
						uv_gpio_get_port(gpio)) & ~(1 << uv_gpio_get_pin(gpio)));
		Chip_GPIOINT_SetIntFalling(LPC_GPIOINT, uv_gpio_get_port(gpio),
				Chip_GPIOINT_GetIntFalling(LPC_GPIOINT,
						uv_gpio_get_port(gpio)) & ~(1 << uv_gpio_get_pin(gpio)));
	}
}



void GPIO_IRQHandler(void) {
	uint32_t p0_pins = Chip_GPIOINT_GetStatusFalling(LPC_GPIOINT, 0) |
			Chip_GPIOINT_GetStatusRising(LPC_GPIOINT, 0);
	for (uint8_t i = 0; i < 32; i++) {
		if (!Chip_GPIOINT_IsIntPending(LPC_GPIOINT, 0)) {
			break;
		}
		if (p0_pins | (1 << i)) {
			callback(P0_0 + i);
			Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, 0, (1 << i));
		}
	}
	uint32_t p2_pins = Chip_GPIOINT_GetStatusFalling(LPC_GPIOINT, 2) |
			Chip_GPIOINT_GetStatusRising(LPC_GPIOINT, 2);
	for (uint8_t i = 0; i < 32; i++) {
		if (!Chip_GPIOINT_IsIntPending(LPC_GPIOINT, 2)) {
			break;
		}
		if (p2_pins | (1 << i)) {
			callback(P2_0 + i);
			Chip_GPIOINT_ClearIntStatus(LPC_GPIOINT, 2, (1 << i));
		}
	}
}


void UV_GPIO_CONFIGURE(uv_gpios_e gpio, uint32_t input_config) {
	switch (gpio) {
	case P0_12 ... P0_13:
	case P0_23 ... P0_26:
	case P1_30 ... P1_31:
	case P0_7 ... P0_9:
	case P1_5 ... P1_7:
	case P1_14:
	case P1_16 ... P1_17:
		input_config |= MD_ANA_DIS;
	break;
	default:
		break;
	}
	LPC_IOCON->p[uv_gpio_get_port(gpio)][uv_gpio_get_pin(gpio)] = input_config;
}


