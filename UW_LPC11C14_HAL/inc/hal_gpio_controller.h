/*
 * hal_gpio_controller.h
 *
 *  Created on: Nov 16, 2015
 *      Author: usevolt
 */

#ifndef HAL_GPIO_CONTROLLER_H_
#define HAL_GPIO_CONTROLLER_H_



#include <stdbool.h>

/// @brief: provides an interface for initializing input and output gpios
/// as well as reading and writing to them

#define GPIO_PORT_MASK	0xC0000000
#define GPIO_PIN_MASK	0x3FFFFFFF

#define GPIO_PORT_0		(0x0 << 30)
#define GPIO_PORT_1		(0x1 << 30)
#define GPIO_PORT_2		(0x2 << 30)
#define GPIO_PORT_3		(0x3 << 30)


/// @brief: Macro for getting GPIO pin number from hal_gpios_e
#define PIN(x)		(x & GPIO_PIN_MASK)
/// @brief: Macro for getting port number from hal_gpios_e
#define PORT(x)		((x & GPIO_PORT_MASK) >> 30)

typedef enum {
	PIO0_0 = GPIO_PORT_0 | 0,
	PIO0_1 = GPIO_PORT_0 | 1,
	PIO0_2 = GPIO_PORT_0 | 2,
	PIO0_3 = GPIO_PORT_0 | 3,
	PIO0_4 = GPIO_PORT_0 | 4,
	PIO0_5 = GPIO_PORT_0 | 5,
	PIO0_6 = GPIO_PORT_0 | 6,
	PIO0_7 = GPIO_PORT_0 | 7,
	PIO0_8 = GPIO_PORT_0 | 8,
	PIO0_9 = GPIO_PORT_0 | 9,
	PIO0_10 = GPIO_PORT_0 | 10,
	PIO0_11 = GPIO_PORT_0 | 11,
	PIO1_0 = GPIO_PORT_1 | 0,
	PIO1_1 = GPIO_PORT_1 | 1,
	PIO1_2 = GPIO_PORT_1 | 2,
	PIO1_3 = GPIO_PORT_1 | 3,
	PIO1_4 = GPIO_PORT_1 | 4,
	PIO1_5 = GPIO_PORT_1 | 5,
	PIO1_6 = GPIO_PORT_1 | 6,
	PIO1_7 = GPIO_PORT_1 | 7,
	PIO1_8 = GPIO_PORT_1 | 8,
	PIO1_9 = GPIO_PORT_1 | 9,
	PIO1_10 = GPIO_PORT_1 | 10,
	PIO1_11 = GPIO_PORT_1 | 11,
	PIO2_0 = GPIO_PORT_2 | 0,
	PIO2_1 = GPIO_PORT_2 | 1,
	PIO2_2 = GPIO_PORT_2 | 2,
	PIO2_3 = GPIO_PORT_2 | 3,
	PIO2_4 = GPIO_PORT_2 | 4,
	PIO2_5 = GPIO_PORT_2 | 5,
	PIO2_6 = GPIO_PORT_2 | 6,
	PIO2_7 = GPIO_PORT_2 | 7,
	PIO2_8 = GPIO_PORT_2 | 8,
	PIO2_9 = GPIO_PORT_2 | 9,
	PIO2_10 = GPIO_PORT_2 | 10,
	PIO2_11 = GPIO_PORT_2 | 11,
	PIO3_0 = GPIO_PORT_3 | 0,
	PIO3_1 = GPIO_PORT_3 | 1,
	PIO3_2 = GPIO_PORT_3 | 2,
	PIO3_3 = GPIO_PORT_3 | 3
} hal_gpios_e;

typedef enum {
	PULL_UP_DISABLED = 0,
	PULL_DOWN_ENABLED = (1 << 3),
	PULL_UP_ENABLED = (1 << 4),
	REPEATER_MODE = (0b11 << 3),
	HYSTERESIS_ENABLED = (1 << 5)
} hal_gpio_input_config_e;

typedef enum {
	INT_EDGE_SENSITIVE = 0x0,
	INT_LEVEL_SENSITIVE = 0x1,
	INT_RISING_EDGE = (0x1 << 1),
	INT_FALLING_EDGE = (0x0 << 1),
	INT_BOTH_EDGES = (0x2 << 1),
	INT_HIGH_LEVEL = (0x1 << 1),
	INT_LOW_LEVEL = (0x0 << 1),
	INT_DISABLE = 0xffff
} hal_gpio_interrupt_config_e;



/// @brief: Initializes gpio as an output
/// @param gpio The gpio pin which will be configured
/// @param initial value The initial value which will be written to the gpio pin.
/// @example: hal_gpio_init_output(PIO1_9, true);
void hal_gpio_init_output(hal_gpios_e gpio, bool initial_value);


/// @brief: Initializes the gpio pin as an input.
/// @param gpio The gpio pin which will be configured
/// @param configurations OR'red configurations for the pin. This will be written to the gpio's IOCON register.
// Additional settings such as the mode of the pin can be configured as well.
/// @param int_configurations: OR'red interrupt configurations on the pin
/// @example: hal_gpio_init_input(PIO1_9, PULL_UP_ENABLED | HYSTERESIS_ENABLED);
void hal_gpio_init_input(hal_gpios_e gpio, hal_gpio_input_config_e configurations,
		hal_gpio_interrupt_config_e int_configurations);




/// @brief: Register a callback function to be called when external interrupt occurs
/// @param callback_function A function pointer to the callback function which will be called
void hal_gpio_set_interrupt_callback(void (*callback_function)(hal_gpios_e));

/// @brief: Sets the output value on an output gpio pin
/// @pre: hal_gpio_init_output should have been called for this pin
void hal_gpio_set_pin(hal_gpios_e gpio, bool value);

/// @brief: Toggles the state of the output gpio pin
/// @pre: hal_gpio_init_output should have been called for this pin
void hal_gpio_toggle_pin(hal_gpios_e gpio);

/// @brief: Gets the input from the input gpio pin.
/// @pre: hal_gpio_init_input should have been called on this pin.
bool hal_gpio_get_pin(hal_gpios_e gpio);


#endif /* HAL_GPIO_CONTROLLER_H_ */
