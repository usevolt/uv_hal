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

#ifndef UW_GPIO_H_
#define UW_GPIO_H_


#include "uv_hal_config.h"


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "uv_errors.h"



#if CONFIG_TARGET_LPC15XX
#include "chip.h"
#include "gpio_15xx.h"
#include "uv_gpio_lpc1549.h"
#elif CONFIG_TARGET_LPC40XX
#include "chip.h"
#include "gpio_17xx_40xx.h"
#include "gpioint_17xx_40xx.h"
#include "uv_gpio_lpc4078.h"
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#include "uv_gpio_linuxwin.h"
#endif


/// @brief: provides an interface for initializing input and output gpios
/// as well as reading and writing to them




typedef enum {
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	HYSTERESIS_ENABLED = (1 << 5)
} uv_gpio_input_config_e;

typedef enum {
	INT_RISING_EDGE = (0x1 << 0),
	INT_FALLING_EDGE = (0x1 << 1),
	INT_BOTH_EDGES = INT_RISING_EDGE | INT_FALLING_EDGE,
	INT_LEVEL_HIGH = (0x1 << 2),
	INT_LEVEL_LOW = (0x1 << 3),
	INT_DISABLE = 0
} uv_gpio_interrupt_config_e;



#define UV_GPIO_PORT(gpio) \
	(CAT(CAT(GPIO_, gpio), _port))
#define UV_GPIO_PIN(gpio) \
	(CAT(CAT(GPIO_, gpio), _pin))

/// @brief: Sets the state of an output pin
///
/// @param gpio: uv_gpios_e pin to be configured
#if CONFIG_TARGET_LPC15XX || CONFIG_TARGET_LPC40XX
#define UV_GPIO_SET(gpio, value)  Chip_GPIO_SetPinState(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio), value)
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define UV_GPIO_SET(gpio, value)
#else
#error "not implemented"
#endif

/// @brief: Toggles an output pin
///
/// @param gpio: uv_gpios_e pin to be configured
#if CONFIG_TARGET_LPC40XX
#define UV_GPIO_TOGGLE(gpio) Chip_GPIO_SetPinState(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio), \
		!Chip_GPIO_GetPinState(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio)))
#elif CONFIG_TARGET_LPC15XX
#define UV_GPIO_TOGGLE(gpio) Chip_GPIO_SetPinToggle(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio))
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define UV_GPIO_TOGGLE(gpio)
#else
#error "not implemented"
#endif


/// @brief: Returns the state of the input GPIO pin
///
/// @param gpio: uv_gpios_e pin to be configured
#if CONFIG_TARGET_LPC15XX || CONFIG_TARGET_LPC40XX
#define UV_GPIO_GET(gpio)		Chip_GPIO_GetPinState(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio))
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define UV_GPIO_GET(gpio)
#else
#error "not implemented"
#endif






/// @brief: Configures the IOCONFIG register with gpio_input_configurations_e values
///
/// @note: Only to be used inside this HAL library!
#if CONFIG_TARGET_LPC15XX
#define UV_GPIO_CONFIGURE(gpio, input_config)\
	LPC_IOCON->PIO[uv_gpio_get_port(gpio)][uv_gpio_get_pin(gpio)] = input_config
#elif CONFIG_TARGET_LPC40XX
void UV_GPIO_CONFIGURE(uv_gpios_e gpio, uint32_t input_config);
#else
#define UV_GPIO_CONFIGURE(gpio, input_config)\
	CAT(CAT(GPIO_, gpio), _config)(input_config)
#endif




/// @brief: Register a callback function to be called when external interrupt occurs
/// @param callback_function: A function pointer to the callback function which will be called
/// The callback function takes 2 parameter: user pointer (see uv_utilities.h) and gpio pin
/// which caused the interrupt.
void uv_gpio_interrupt_init(void (*callback_function)(uv_gpios_e));




uint8_t uv_gpio_get_port(uv_gpios_e gpio);

uint8_t uv_gpio_get_pin(uv_gpios_e gpio);

#if CONFIG_TARGET_LPC15XX

static inline bool uv_gpio_get(uv_gpios_e gpio) {
	return Chip_GPIO_GetPinState(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
}

static inline void uv_gpio_set(uv_gpios_e gpio, bool value) {
	Chip_GPIO_SetPinState(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio), value);
}

static inline void uv_gpio_toggle(uv_gpios_e gpio) {
	Chip_GPIO_SetPinToggle(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
}

static inline void uv_gpio_init_input(uv_gpios_e gpio, uint32_t confs) {
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
	UV_GPIO_CONFIGURE(gpio, confs);
}

static inline void uv_gpio_init_output(uv_gpios_e gpio, bool value) {
	UV_GPIO_CONFIGURE(gpio, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
	uv_gpio_set(gpio, value);
}

#elif CONFIG_TARGET_LPC40XX

static inline bool uv_gpio_get(uv_gpios_e gpio) {
	return Chip_GPIO_GetPinState(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
}

static inline void uv_gpio_set(uv_gpios_e gpio, bool value) {
	Chip_GPIO_SetPinState(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio), value);
}

static inline void uv_gpio_init_input(uv_gpios_e gpio, uint32_t confs) {
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
	UV_GPIO_CONFIGURE(gpio, confs);
}

static inline void uv_gpio_init_output(uv_gpios_e gpio, bool value) {
	UV_GPIO_CONFIGURE(gpio, 0);
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, uv_gpio_get_port(gpio), uv_gpio_get_pin(gpio));
	uv_gpio_set(gpio, value);
}


#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN

static inline bool uv_gpio_get(uv_gpios_e gpio) {
	return true;
}

static inline void uv_gpio_set(uv_gpios_e gpio, bool value) { }

static inline void uv_gpio_toggle(uv_gpios_e gpio) { }

static inline void uv_gpio_init_input(uv_gpios_e gpio, uint32_t confs) { }

static inline void uv_gpio_init_output(uv_gpios_e gpio, bool value) { }

#endif


/// @brief: Enables interrupts on pin **gpio**
///
/// @note: There is a maximum limit of enabled interrupt pins depending on the hardware
///
/// @return: ERR_HARDWARE_NOT_SUPPORTED if the maximum number of int pins has been
/// reached, ERR_NO_NEW_VALUES if the current pin already has int enabled,
/// otherwise ERR_NONE.
uv_errors_e uv_gpio_enable_int(uv_gpios_e gpio, uv_gpio_interrupt_config_e confs);

/// @brief: Disables interrupts on pin **gpio**
void uv_gpio_disable_int(uv_gpios_e gpio);



#endif /* UW_GPIO_H_ */
