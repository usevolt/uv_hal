/*
 * uv_gpio.h
 *
 *  Created on: Nov 16, 2015
 *      Author: usevolt
 */

#ifndef UW_GPIO_H_
#define UW_GPIO_H_


#include "uv_hal_config.h"


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "uv_errors.h"



#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#include "uv_gpio_lpc1785.h"
#elif CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#include "uv_gpio_lpc11c14.h"
#elif CONFIG_TARGET_LPC1549
#include "chip.h"
#include "gpio_15xx.h"
#include "uv_gpio_lpc1549.h"
#endif


/// @brief: provides an interface for initializing input and output gpios
/// as well as reading and writing to them




typedef enum {
#if CONFIG_TARGET_LPC11C14
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	REPEATER_MODE = (0b11 << 3),
	HYSTERESIS_ENABLED = (1 << 5)
#elif CONFIG_TARGET_LPC1785
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	HYSTERESIS_ENABLED = (1 << 5),
	INVERSE_POLARITY = (1 << 6),
	GLITCH_FILTER_ENABLED = (1 << 8),
	SLEW_CONTROL_ENABLED = 0,
	SLEW_CONTROL_DISABLED = (1 << 9),
	OPEN_DRAIN_MODE_ENABLED = (1 << 10)
#elif CONFIG_TARGET_LPC1549
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	HYSTERESIS_ENABLED = (1 << 5)
#endif
} uv_gpio_input_config_e;

typedef enum {
#if CONFIG_TARGET_LPC11C14
	INT_RISING_EDGE = 0x1,
	INT_FALLING_EDGE = 0x2,
	INT_BOTH_EDGES = INT_RISING_EDGE | INT_FALLING_EDGE,
	INT_DISABLE = 0
#elif CONFIG_TARGET_LPC1785
	INT_RISING_EDGE = (0x1 << 0),
	INT_FALLING_EDGE = (0x1 << 1),
	INT_BOTH_EDGES = (0x2),
	INT_DISABLE = 0
#elif CONFIG_TARGET_LPC1549
	INT_RISING_EDGE = (0x1 << 0),
	INT_FALLING_EDGE = (0x1 << 1),
	INT_BOTH_EDGES = (0x2),
	INT_DISABLE = 0
#endif
} uv_gpio_interrupt_config_e;





/// @brief: Register a callback function to be called when external interrupt occurs
/// @param callback_function: A function pointer to the callback function which will be called
/// The callback function takes 2 parameter: user pointer (see uv_utilities.h) and gpio pin
/// which caused the interrupt.
void uv_gpio_add_interrupt_callback(void (*callback_function)(void * user_ptr, uv_gpios_e));


#define UV_GPIO_PORT(gpio) \
	(CAT(CAT(GPIO_, gpio), _port))
#define UV_GPIO_PIN(gpio) \
	(CAT(CAT(GPIO_, gpio), _pin))

/// @brief: Sets the state of an output pin
///
/// @param gpio: uv_gpios_e pin to be configured
#if CONFIG_TARGET_LPC1785
#define UV_GPIO_SET(gpio, value)	\
	(port(CAT(CAT(GPIO_, gpio), _port))->PIN = (port(CAT(CAT(GPIO_, gpio), _port))->PIN \
			& ~(1 << CAT(CAT(GPIO_, gpio), _pin))) | (value << CAT(CAT(GPIO_, gpio), _pin)))
#elif CONFIG_TARGET_LPC11C14
#define UV_GPIO_SET(gpio, value)  \
	(port(CAT(CAT(GPIO_, gpio), _port))->DATA = (port(CAT(CAT(GPIO_, gpio), _port))->DATA \
			& ~(1 << CAT(CAT(GPIO_, gpio), _pin))) | (value << CAT(CAT(GPIO_, gpio), _pin)))
#elif CONFIG_TARGET_LPC1549
#define UV_GPIO_SET(gpio, value)  Chip_GPIO_SetPinState(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio), value)
#else
#error "not implemented"
#endif

/// @brief: Toggles an output pin
///
/// @param gpio: uv_gpios_e pin to be configured
#if CONFIG_TARGET_LPC1785
#define UV_GPIO_TOGGLE(gpio) \
	(port(CAT(CAT(GPIO_, gpio), _port))->PIN ^= (1 << CAT(CAT(GPIO_, gpio), _pin)))
#elif CONFIG_TARGET_LPC11C14
#define UV_GPIO_TOGGLE(gpio) \
	(port(CAT(CAT(GPIO_, gpio), _port))->DATA ^= (1 << CAT(CAT(GPIO_, gpio), _pin)))
#elif CONFIG_TARGET_LPC1549
#define UV_GPIO_TOGGLE(gpio) Chip_GPIO_SetPinToggle(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio))
#else
#error "not implemented"
#endif


/// @brief: Returns the state of the input GPIO pin
///
/// @param gpio: uv_gpios_e pin to be configured
#if CONFIG_TARGET_LPC1785
#define UV_GPIO_GET(gpio) \
	((port(CAT(CAT(GPIO_, gpio), _port))->PIN & (1 << UV_GPIO_PIN(gpio))) >> UV_GPIO_PIN(gpio))
#elif CONFIG_TARGET_LPC11C14
#define UV_GPIO_GET(gpio) \
	((port(CAT(CAT(GPIO_, gpio), _port))->DATA & (1 << UV_GPIO_PIN(gpio))) >> UV_GPIO_PIN(gpio))
#elif CONFIG_TARGET_LPC1549
#define UV_GPIO_GET		Chip_GPIO_GetPinState(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio))
#else
#error "not implemented"
#endif



/// @brief: Initializes any GPIO pin as an output
///
/// @param gpio: uv_gpios_e pin to be configured
/// @param initial_val: True of false, the initial output value
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785
#define UV_GPIO_INIT_OUTPUT(gpio, initial_val) \
	UV_GPIO_CONFIGURE(gpio, 0); \
	port(CAT(CAT(GPIO_, gpio), _port))->DIR |= (1 << CAT(CAT(GPIO_, gpio), _pin)); \
	UV_GPIO_SET(gpio, initial_val)
#elif CONFIG_TARGET_LPC1549
#define UV_GPIO_INIT_OUTPUT(gpio, initial_val) \
	UV_GPIO_CONFIGURE(gpio, 0); \
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio)); \
	UV_GPIO_SET(gpio, initial_val)
#endif



/// @brief: Initializes a GPIO input pin with pin change interrupts. Only PORT 0 & 2
/// pins can be configured with this.
///
/// @note: If this macro causes a compile error, the issue is most probably that
/// GPIO port doesn't support interrupt. Only port 0 and 2 support interrupts.
///
/// @param gpio: uv_gpios_e, gpio pin to be configured
/// @param confs: OR'red uv_gpio_input_config_e configuration flags.
/// @param interrupt_confs: OR'red uv_gpio_interrupt_config_e configuration flags.
#if CONFIG_TARGET_LPC1785
#define UV_GPIO_INIT_INPUT_INT(gpio, confs, interrupt_confs) \
	port(CAT(CAT(GPIO_, gpio), _port))->DIR &= ~(1 << CAT(CAT(GPIO_, gpio), _pin)); \
	UV_GPIO_CONFIGURE(gpio, confs); \
	CAT(CAT(LPC_GPIOINT->IO, CAT(CAT(GPIO_, gpio), _port)), IntEnF) |= ((interrupt_conf & (~1)) << CAT(CAT(GPIO_, gpio), _pin)); \
	CAT(CAT(LPC_GPIOINT->IO, CAT(CAT(GPIO_, gpio), _port)), IntEnR) |= ((interrupt_conf & (~(1 << 1))) << CAT(CAT(GPIO_, gpio), _pin))
#elif CONFIG_TARGET_LPC11C14
#define UV_GPIO_INIT_INPUT_INT(gpio, confs, interrupt_confs) \
	UV_GPIO_INIT_INPUT(gpio, confs); \
	port(CAT(CAT(GPIO_, gpio), _port))->IC |= (1 << UV_GPIO_PIN(gpio)); \
	port(CAT(CAT(GPIO_, gpio), _port))->IS &= ~(1 << UV_GPIO_PIN(gpio)); \
	if ((interrupt_confs & INT_BOTH_EDGES) == INT_BOTH_EDGES) { \
		port(CAT(CAT(GPIO_, gpio), _port))->IBE |= (1 << UV_GPIO_PIN(gpio)); \
	} else { \
		port(CAT(CAT(GPIO_, gpio), _port))->IEV |= \
			(((interrupt_confs & INT_FALLING_EDGE) ? 0 : 1) << UV_GPIO_PIN(gpio)); \
	} \
	port(CAT(CAT(GPIO_, gpio), _port))->IE |= (((interrupt_confs) ? 1 : 0) << UV_GPIO_PIN(gpio))
#elif CONFIG_TARGET_LPC1549
// todo: this macro is not ready!
#endif

/// @brief: Initializes any GPIO pin as an input
///
/// @param gpio: uv_gpios_e, gpio pin to be configured
/// @param confs: OR'red uv_gpio_input_config_e configuration flags.
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785
#define UV_GPIO_INIT_INPUT(gpio, confs) \
	port(CAT(CAT(GPIO_, gpio), _port))->DIR &= ~(1 << CAT(CAT(GPIO_, gpio), _pin)); \
	UV_GPIO_CONFIGURE(gpio, confs)
#elif CONFIG_TARGET_LPC1549
#define UV_GPIO_INIT_INPUT(gpio, confs) \
	Chip_GPIO_SetPinDIRInput(LPC_GPIO, UV_GPIO_PORT(gpio), UV_GPIO_PIN(gpio)); \
	UV_GPIO_CONFIGURE(gpio, confs)
#endif



/// @brief: Configures the IOCONFIG register with gpio_input_configurations_e values
///
/// @note: Only to be used inside this HAL library!
#if CONFIG_TARGET_LPC1549
#define UV_GPIO_CONFIGURE(gpio, input_config)\
	LPC_IOCON->PIO[uv_gpio_get_port(gpio)][uv_gpio_get_pin(gpio)] = __TABLE1(input_config)
#else
#define UV_GPIO_CONFIGURE(gpio, input_config)\
	CAT(CAT(GPIO_, gpio), _config)(input_config)
#endif



#if CONFIG_TARGET_LPC1549

uint8_t uv_gpio_get_port(uv_gpios_e gpio);

uint8_t uv_gpio_get_pin(uv_gpios_e gpio);

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





#endif

#endif /* UW_GPIO_H_ */
