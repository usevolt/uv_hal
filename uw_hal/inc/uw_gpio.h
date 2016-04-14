/*
 * uw_gpio.h
 *
 *  Created on: Nov 16, 2015
 *      Author: usevolt
 */

#ifndef UW_GPIO_H_
#define UW_GPIO_H_


#include "uw_hal_config.h"


#include <stdbool.h>
#include <stdint.h>
#include "uw_errors.h"



#if CONFIG_TARGET_LPC178X
#define UW_GPIO_SET_MODE(port, pin, mode)	LPC_IOCON->P ## port ## _ ## pin &= 0b111; \
											LPC_IOCON->P ## port ## _ ## pin |= mode
#endif



/// @brief: provides an interface for initializing input and output gpios
/// as well as reading and writing to them


#if CONFIG_TARGET_LPC11CXX
#define GPIO_PORT_MASK	0xC0000000
#define GPIO_PIN_MASK	0x3FFFFFFF

#define PIN(x)		(x & GPIO_PIN_MASK)
#define PORT(x)		((x & GPIO_PORT_MASK))

#define GPIO_PORT_0		(0x0 << 30)
#define GPIO_PORT_1		(0x1 << 30)
#define GPIO_PORT_2		(0x2 << 30)
#define GPIO_PORT_3		(0x3 << 30)

#elif CONFIG_TARGET_LPC178X
#define GPIO_PORT_MASK	0xE0000000
#define GPIO_PIN_MASK	0x1FFFFFFF

#define PIN(x)		(x & GPIO_PIN_MASK)
#define PORT(x)		(x & GPIO_PORT_MASK)

#define GPIO_PORT_0		(0x0 << 29)
#define GPIO_PORT_1		(0x1 << 29)
#define GPIO_PORT_2		(0x2 << 29)
#define GPIO_PORT_3		(0x3 << 29)
#define GPIO_PORT_4		(0x4 << 29)
#define GPIO_PORT_5		(0x5 << 29)

#else
#error "No device defined"
#endif



typedef enum {
#if CONFIG_TARGET_LPC11CXX
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	REPEATER_MODE = (0b11 << 3),
	HYSTERESIS_ENABLED = (1 << 5)
#elif CONFIG_TARGET_LPC178X
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	HYSTERESIS_ENABLED = (1 << 5),
	INVERSE_POLARITY = (1 << 6),
	GLITCH_FILTER_ENABLED = (1 << 8),
	SLEW_CONTROL_ENABLED = 0,
	SLEW_CONTROL_DISABLED = (1 << 9),
	OPEN_DRAIN_MODE_ENABLED = (1 << 10)
#endif
} uw_gpio_input_config_e;

typedef enum {
	INT_EDGE_SENSITIVE = 0x0,
#if CONFIG_TARGET_LPC11CXX
	INT_LEVEL_SENSITIVE = 0x1,
	INT_RISING_EDGE = (0x1 << 1),
	INT_FALLING_EDGE = (0x0 << 1),
	INT_BOTH_EDGES = (0x2 << 1),
	INT_HIGH_LEVEL = (0x1 << 1),
	INT_LOW_LEVEL = (0x0 << 1),
	INT_DISABLE = 0xffff
#elif CONFIG_TARGET_LPC178X
	INT_RISING_EDGE = (0x1 << 0),
	INT_FALLING_EDGE = (0x1 << 1),
	INT_BOTH_EDGES = (0x2),
	INT_DISABLE = 0
#endif
} uw_gpio_interrupt_config_e;

typedef enum uw_gpios_e uw_gpios_e;


/// @brief: Initializes gpio as an output
/// @param gpio The gpio pin which will be configured
/// @param initial value The initial value which will be written to the gpio pin.
/// @example: uw_gpio_init_output(PIO1_9, true);
uw_errors_e uw_gpio_init_output(uw_gpios_e gpio, bool initial_value);



/// @brief: Initializes the gpio pin as an input.
///
/// @note: Refer to datasheet for pins which can be used as interrupt sources.
///
/// @param gpio The gpio pin which will be configured
/// @param configurations OR'red configurations for the pin. This will be written to the gpio's IOCON register.
// Additional settings such as the mode of the pin can be configured as well.
/// @param int_configurations: OR'red interrupt configurations on the pin
///
/// @example: uw_gpio_init_input(PIO1_9, PULL_UP_ENABLED | HYSTERESIS_ENABLED, INT_DISABLE);
uw_errors_e uw_gpio_init_input(uw_gpios_e gpio, uw_gpio_input_config_e configurations,
		uw_gpio_interrupt_config_e int_configurations);



/// @brief: Register a callback function to be called when external interrupt occurs
/// @param callback_function: A function pointer to the callback function which will be called
/// The callback function takes 2 parameter: user pointer (see uw_utilities.h) and gpio pin
/// which caused the interrupt.
void uw_gpio_add_interrupt_callback(void (*callback_function)(void * user_ptr, uw_gpios_e));

/// @brief: Sets the output value on an output gpio pin
/// @pre: uw_gpio_init_output should have been called for this pin
uw_errors_e uw_gpio_set_pin(uw_gpios_e gpio, bool value);

/// @brief: Toggles the state of the output gpio pin
/// @pre: uw_gpio_init_output should have been called for this pin
uw_errors_e uw_gpio_toggle_pin(uw_gpios_e gpio);

/// @brief: Gets the input from the input gpio pin.
/// @pre: uw_gpio_init_input should have been called on this pin.
bool uw_gpio_get_pin(uw_gpios_e gpio);


/// @brief: Returns the pin's IOCON register
///
/// @note: Only to be used inside this library
volatile uint32_t *__uw_gpio_get_iocon(uw_gpios_e gpio);


enum uw_gpios_e {
	PIO_UNDEFINED = 0,
#if CONFIG_TARGET_LPC11CXX
#if (CONFIG_PORT0 | CONFIG_PIO0_0)
	PIO0_0 = GPIO_PORT_0 | 0,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_1)
	PIO0_1 = GPIO_PORT_0 | 1,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_2)
	PIO0_2 = GPIO_PORT_0 | 2,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_3)
	PIO0_3 = GPIO_PORT_0 | 3,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_4)
	PIO0_4 = GPIO_PORT_0 | 4,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_5)
	PIO0_5 = GPIO_PORT_0 | 5,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_6)
	PIO0_6 = GPIO_PORT_0 | 6,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_7)
	PIO0_7 = GPIO_PORT_0 | 7,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_8)
	PIO0_8 = GPIO_PORT_0 | 8,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_9)
	PIO0_9 = GPIO_PORT_0 | 9,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_10)
	PIO0_10 = GPIO_PORT_0 | 10,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_11)
	PIO0_11 = GPIO_PORT_0 | 11,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_0)
	PIO1_0 = GPIO_PORT_1 | 0,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_1)
	PIO1_1 = GPIO_PORT_1 | 1,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_2)
	PIO1_2 = GPIO_PORT_1 | 2,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_3)
	PIO1_3 = GPIO_PORT_1 | 3,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_4)
	PIO1_4 = GPIO_PORT_1 | 4,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_5)
	PIO1_5 = GPIO_PORT_1 | 5,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_6)
	PIO1_6 = GPIO_PORT_1 | 6,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_7)
	PIO1_7 = GPIO_PORT_1 | 7,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_8)
	PIO1_8 = GPIO_PORT_1 | 8,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_9)
	PIO1_9 = GPIO_PORT_1 | 9,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_10)
	PIO1_10 = GPIO_PORT_1 | 10,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_11)
	PIO1_11 = GPIO_PORT_1 | 11,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_0)
	PIO2_0 = GPIO_PORT_2 | 0,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_1)
	PIO2_1 = GPIO_PORT_2 | 1,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_2)
	PIO2_2 = GPIO_PORT_2 | 2,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_3)
	PIO2_3 = GPIO_PORT_2 | 3,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_4)
	PIO2_4 = GPIO_PORT_2 | 4,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_5)
	PIO2_5 = GPIO_PORT_2 | 5,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_6)
	PIO2_6 = GPIO_PORT_2 | 6,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_7)
	PIO2_7 = GPIO_PORT_2 | 7,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_8)
	PIO2_8 = GPIO_PORT_2 | 8,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_9)
	PIO2_9 = GPIO_PORT_2 | 9,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_10)
	PIO2_10 = GPIO_PORT_2 | 10,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_11)
	PIO2_11 = GPIO_PORT_2 | 11,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_0)
	PIO3_0 = GPIO_PORT_3 | 0,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_1)
	PIO3_1 = GPIO_PORT_3 | 1,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_2)
	PIO3_2 = GPIO_PORT_3 | 2,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_3)
	PIO3_3 = GPIO_PORT_3 | 3
#endif
#elif CONFIG_TARGET_LPC178X
#if (CONFIG_PORT0 | CONFIG_PIO0_0)
	PIO0_0 = GPIO_PORT_0 | 0,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_1)
	PIO0_1 = GPIO_PORT_0 | 1,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_2)
	PIO0_2 = GPIO_PORT_0 | 2,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_3)
	PIO0_3 = GPIO_PORT_0 | 3,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_4)
	PIO0_4 = GPIO_PORT_0 | 4,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_5)
	PIO0_5 = GPIO_PORT_0 | 5,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_6)
	PIO0_6 = GPIO_PORT_0 | 6,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_7)
	PIO0_7 = GPIO_PORT_0 | 7,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_8)
	PIO0_8 = GPIO_PORT_0 | 8,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_9)
	PIO0_9 = GPIO_PORT_0 | 9,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_10)
	PIO0_10 = GPIO_PORT_0 | 10,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_11)
	PIO0_11 = GPIO_PORT_0 | 11,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_12)
	PIO0_12 = GPIO_PORT_0 | 12,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_13)
	PIO0_13 = GPIO_PORT_0 | 13,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_14)
	PIO0_14 = GPIO_PORT_0 | 14,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_15)
	PIO0_15 = GPIO_PORT_0 | 15,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_16)
	PIO0_16 = GPIO_PORT_0 | 16,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_17)
	PIO0_17 = GPIO_PORT_0 | 17,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_18)
	PIO0_18 = GPIO_PORT_0 | 18,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_19)
	PIO0_19 = GPIO_PORT_0 | 19,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_20)
	PIO0_20 = GPIO_PORT_0 | 20,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_21)
	PIO0_21 = GPIO_PORT_0 | 21,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_22)
	PIO0_22 = GPIO_PORT_0 | 22,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_23)
	PIO0_23 = GPIO_PORT_0 | 23,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_24)
	PIO0_24 = GPIO_PORT_0 | 24,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_25)
	PIO0_25 = GPIO_PORT_0 | 25,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_26)
	PIO0_26 = GPIO_PORT_0 | 26,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_27)
	PIO0_27 = GPIO_PORT_0 | 27,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_28)
	PIO0_28 = GPIO_PORT_0 | 28,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_29)
	PIO0_29 = GPIO_PORT_0 | 29,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_30)
	PIO0_30 = GPIO_PORT_0 | 30,
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_31)
	PIO0_31 = GPIO_PORT_0 | 31,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_0)
	PIO1_0 = GPIO_PORT_1 | 0,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_1)
	PIO1_1 = GPIO_PORT_1 | 1,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_2)
	PIO1_2 = GPIO_PORT_1 | 2,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_3)
	PIO1_3 = GPIO_PORT_1 | 3,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_4)
	PIO1_4 = GPIO_PORT_1 | 4,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_5)
	PIO1_5 = GPIO_PORT_1 | 5,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_6)
	PIO1_6 = GPIO_PORT_1 | 6,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_7)
	PIO1_7 = GPIO_PORT_1 | 7,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_8)
	PIO1_8 = GPIO_PORT_1 | 8,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_9)
	PIO1_9 = GPIO_PORT_1 | 9,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_10)
	PIO1_10 = GPIO_PORT_1 | 10,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_11)
	PIO1_11 = GPIO_PORT_1 | 11,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_12)
	PIO1_12 = GPIO_PORT_1 | 12,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_13)
	PIO1_13 = GPIO_PORT_1 | 13,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_14)
	PIO1_14 = GPIO_PORT_1 | 14,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_15)
	PIO1_15 = GPIO_PORT_1 | 15,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_16)
	PIO1_16 = GPIO_PORT_1 | 16,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_17)
	PIO1_17 = GPIO_PORT_1 | 17,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_18)
	PIO1_18 = GPIO_PORT_1 | 18,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_19)
	PIO1_19 = GPIO_PORT_1 | 19,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_20)
	PIO1_20 = GPIO_PORT_1 | 20,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_21)
	PIO1_21 = GPIO_PORT_1 | 21,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_22)
	PIO1_22 = GPIO_PORT_1 | 22,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_23)
	PIO1_23 = GPIO_PORT_1 | 23,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_24)
	PIO1_24 = GPIO_PORT_1 | 24,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_25)
	PIO1_25 = GPIO_PORT_1 | 25,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_26)
	PIO1_26 = GPIO_PORT_1 | 26,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_27)
	PIO1_27 = GPIO_PORT_1 | 27,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_28)
	PIO1_28 = GPIO_PORT_1 | 28,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_29)
	PIO1_29 = GPIO_PORT_1 | 29,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_30)
	PIO1_30 = GPIO_PORT_1 | 30,
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_31)
	PIO1_31 = GPIO_PORT_1 | 31,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_0)
	PIO2_0 = GPIO_PORT_2 | 0,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_1)
	PIO2_1 = GPIO_PORT_2 | 1,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_2)
	PIO2_2 = GPIO_PORT_2 | 2,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_3)
	PIO2_3 = GPIO_PORT_2 | 3,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_4)
	PIO2_4 = GPIO_PORT_2 | 4,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_5)
	PIO2_5 = GPIO_PORT_2 | 5,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_6)
	PIO2_6 = GPIO_PORT_2 | 6,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_7)
	PIO2_7 = GPIO_PORT_2 | 7,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_8)
	PIO2_8 = GPIO_PORT_2 | 8,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_9)
	PIO2_9 = GPIO_PORT_2 | 9,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_10)
	PIO2_10 = GPIO_PORT_2 | 10,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_11)
	PIO2_11 = GPIO_PORT_2 | 11,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_12)
	PIO2_12 = GPIO_PORT_2 | 12,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_13)
	PIO2_13 = GPIO_PORT_2 | 13,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_14)
	PIO2_14 = GPIO_PORT_2 | 14,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_15)
	PIO2_15 = GPIO_PORT_2 | 15,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_16)
	PIO2_16 = GPIO_PORT_2 | 16,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_17)
	PIO2_17 = GPIO_PORT_2 | 17,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_18)
	PIO2_18 = GPIO_PORT_2 | 18,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_19)
	PIO2_19 = GPIO_PORT_2 | 19,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_20)
	PIO2_20 = GPIO_PORT_2 | 20,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_21)
	PIO2_21 = GPIO_PORT_2 | 21,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_22)
	PIO2_22 = GPIO_PORT_2 | 22,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_23)
	PIO2_23 = GPIO_PORT_2 | 23,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_24)
	PIO2_24 = GPIO_PORT_2 | 24,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_25)
	PIO2_25 = GPIO_PORT_2 | 25,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_26)
	PIO2_26 = GPIO_PORT_2 | 26,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_27)
	PIO2_27 = GPIO_PORT_2 | 27,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_28)
	PIO2_28 = GPIO_PORT_2 | 28,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_29)
	PIO2_29 = GPIO_PORT_2 | 29,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_30)
	PIO2_30 = GPIO_PORT_2 | 30,
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_31)
	PIO2_31 = GPIO_PORT_2 | 31,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_0)
	PIO3_0 = GPIO_PORT_3 | 0,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_1)
	PIO3_1 = GPIO_PORT_3 | 1,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_2)
	PIO3_2 = GPIO_PORT_3 | 2,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_3)
	PIO3_3 = GPIO_PORT_3 | 3,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_4)
	PIO3_4 = GPIO_PORT_3 | 4,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_5)
	PIO3_5 = GPIO_PORT_3 | 5,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_6)
	PIO3_6 = GPIO_PORT_3 | 6,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_7)
	PIO3_7 = GPIO_PORT_3 | 7,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_8)
	PIO3_8 = GPIO_PORT_3 | 8,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_9)
	PIO3_9 = GPIO_PORT_3 | 9,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_10)
	PIO3_10 = GPIO_PORT_3 | 10,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_11)
	PIO3_11 = GPIO_PORT_3 | 11,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_12)
	PIO3_12 = GPIO_PORT_3 | 12,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_13)
	PIO3_13 = GPIO_PORT_3 | 13,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_14)
	PIO3_14 = GPIO_PORT_3 | 14,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_15)
	PIO3_15 = GPIO_PORT_3 | 15,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_16)
	PIO3_16 = GPIO_PORT_3 | 16,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_17)
	PIO3_17 = GPIO_PORT_3 | 17,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_18)
	PIO3_18 = GPIO_PORT_3 | 18,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_19)
	PIO3_19 = GPIO_PORT_3 | 19,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_20)
	PIO3_20 = GPIO_PORT_3 | 20,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_21)
	PIO3_21 = GPIO_PORT_3 | 21,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_22)
	PIO3_22 = GPIO_PORT_3 | 22,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_23)
	PIO3_23 = GPIO_PORT_3 | 23,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_24)
	PIO3_24 = GPIO_PORT_3 | 24,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_25)
	PIO3_25 = GPIO_PORT_3 | 25,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_26)
	PIO3_26 = GPIO_PORT_3 | 26,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_27)
	PIO3_27 = GPIO_PORT_3 | 27,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_28)
	PIO3_28 = GPIO_PORT_3 | 28,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_29)
	PIO3_29 = GPIO_PORT_3 | 29,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_30)
	PIO3_30 = GPIO_PORT_3 | 30,
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_31)
	PIO3_31 = GPIO_PORT_3 | 31,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_0)
	PIO4_0 = GPIO_PORT_4 | 0,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_1)
	PIO4_1 = GPIO_PORT_4 | 1,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_2)
	PIO4_2 = GPIO_PORT_4 | 2,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_3)
	PIO4_3 = GPIO_PORT_4 | 3,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_4)
	PIO4_4 = GPIO_PORT_4 | 4,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_5)
	PIO4_5 = GPIO_PORT_4 | 5,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_6)
	PIO4_6 = GPIO_PORT_4 | 6,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_7)
	PIO4_7 = GPIO_PORT_4 | 7,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_8)
	PIO4_8 = GPIO_PORT_4 | 8,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_9)
	PIO4_9 = GPIO_PORT_4 | 9,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_10)
	PIO4_10 = GPIO_PORT_4 | 10,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_11)
	PIO4_11 = GPIO_PORT_4 | 11,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_12)
	PIO4_12 = GPIO_PORT_4 | 12,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_13)
	PIO4_13 = GPIO_PORT_4 | 13,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_14)
	PIO4_14 = GPIO_PORT_4 | 14,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_15)
	PIO4_15 = GPIO_PORT_4 | 15,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_16)
	PIO4_16 = GPIO_PORT_4 | 16,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_17)
	PIO4_17 = GPIO_PORT_4 | 17,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_18)
	PIO4_18 = GPIO_PORT_4 | 18,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_19)
	PIO4_19 = GPIO_PORT_4 | 19,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_20)
	PIO4_20 = GPIO_PORT_4 | 20,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_21)
	PIO4_21 = GPIO_PORT_4 | 21,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_22)
	PIO4_22 = GPIO_PORT_4 | 22,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_23)
	PIO4_23 = GPIO_PORT_4 | 23,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_24)
	PIO4_24 = GPIO_PORT_4 | 24,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_25)
	PIO4_25 = GPIO_PORT_4 | 25,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_26)
	PIO4_26 = GPIO_PORT_4 | 26,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_27)
	PIO4_27 = GPIO_PORT_4 | 27,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_28)
	PIO4_28 = GPIO_PORT_4 | 28,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_29)
	PIO4_29 = GPIO_PORT_4 | 29,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_30)
	PIO4_30 = GPIO_PORT_4 | 30,
#endif
#if (CONFIG_PORT4 | CONFIG_PIO4_31)
	PIO4_31 = GPIO_PORT_4 | 31,
#endif
#if (CONFIG_PORT5 | CONFIG_PIO5_0)
	PIO5_0 = GPIO_PORT_5 | 0,
#endif
#if (CONFIG_PORT5 | CONFIG_PIO5_1)
	PIO5_1 = GPIO_PORT_5 | 1,
#endif
#if (CONFIG_PORT5 | CONFIG_PIO5_2)
	PIO5_2 = GPIO_PORT_5 | 2,
#endif
#if (CONFIG_PORT5 | CONFIG_PIO5_3)
	PIO5_3 = GPIO_PORT_5 | 3,
#endif
#if (CONFIG_PORT5 | CONFIG_PIO5_4)
	PIO5_4 = GPIO_PORT_5 | 4
#endif

#endif
};


#endif /* UW_GPIO_H_ */
