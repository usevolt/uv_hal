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
#include "uv_errors.h"



#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#include "uv_gpio_lpc1785.h"
#endif



/// @brief: provides an interface for initializing input and output gpios
/// as well as reading and writing to them


#if CONFIG_TARGET_LPC11C14
#define GPIO_PORT_MASK	0xC0000000
#define GPIO_PIN_MASK	0x3FFFFFFF

#define PIN(x)		(x & GPIO_PIN_MASK)
#define PORT(x)		((x & GPIO_PORT_MASK))

#define GPIO_PORT_0		(0x0 << 30)
#define GPIO_PORT_1		(0x1 << 30)
#define GPIO_PORT_2		(0x2 << 30)
#define GPIO_PORT_3		(0x3 << 30)


#error "Note: uv_gpio should be updated to macro-based interface with lpc11c14"
#endif



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
#endif
} uv_gpio_input_config_e;

typedef enum {
	INT_EDGE_SENSITIVE = 0x0,
#if CONFIG_TARGET_LPC11C14
	INT_LEVEL_SENSITIVE = 0x1,
	INT_RISING_EDGE = (0x1 << 1),
	INT_FALLING_EDGE = (0x0 << 1),
	INT_BOTH_EDGES = (0x2 << 1),
	INT_HIGH_LEVEL = (0x1 << 1),
	INT_LOW_LEVEL = (0x0 << 1),
	INT_DISABLE = 0xffff
#elif CONFIG_TARGET_LPC1785
	INT_RISING_EDGE = (0x1 << 0),
	INT_FALLING_EDGE = (0x1 << 1),
	INT_BOTH_EDGES = (0x2),
	INT_DISABLE = 0
#endif
} uv_gpio_interrupt_config_e;


#if CONFIG_TARGET_LPC11C14
/// @brief: Initializes gpio as an output
/// @param gpio The gpio pin which will be configured
/// @param initial value The initial value which will be written to the gpio pin.
/// @example: uv_gpio_init_output(PIO1_9, true);
uv_errors_e uv_gpio_init_output(uv_gpios_e gpio, bool initial_value);



/// @brief: Initializes the gpio pin as an input.
///
/// @note: Refer to datasheet for pins which can be used as interrupt sources.
///
/// @param gpio The gpio pin which will be configured
/// @param configurations OR'red configurations for the pin. This will be written to the gpio's IOCON register.
// Additional settings such as the mode of the pin can be configured as well.
/// @param int_configurations: OR'red interrupt configurations on the pin
///
/// @example: uv_gpio_init_input(PIO1_9, PULL_UP_ENABLED | HYSTERESIS_ENABLED, INT_DISABLE);
uv_errors_e uv_gpio_init_input(uv_gpios_e gpio, uv_gpio_input_config_e configurations,
		uv_gpio_interrupt_config_e int_configurations);
#endif



/// @brief: Register a callback function to be called when external interrupt occurs
/// @param callback_function: A function pointer to the callback function which will be called
/// The callback function takes 2 parameter: user pointer (see uv_utilities.h) and gpio pin
/// which caused the interrupt.
void uv_gpio_add_interrupt_callback(void (*callback_function)(void * user_ptr, uv_gpios_e));




/// @brief: Sets the state of an output pin
///
/// @param gpio: uv_gpios_e pin to be configured
#define uv_gpio_set(gpio, value)	\
	(port(CAT(gpio, _port))->PIN = (port(CAT(gpio, _port))->PIN & ~(1 << CAT(gpio, _pin))) | (value << CAT(gpio, _pin)))


/// @brief: Toggles an output pin
///
/// @param gpio: uv_gpios_e pin to be configured
#define uv_gpio_toggle(gpio) \
	(port(CAT(gpio, _port))->PIN ^= (1 << CAT(gpio, _pin)))


/// @brief: Returns the state of the input GPIO pin
///
/// @param gpio: uv_gpios_e pin to be configured
#define uv_gpio_get(gpio) \
	((CAT(gpio, _port)->PIN & ~(1 << CAT(gpio, _pin))) >> CAT(gpio, _pin))



/// @brief: Initializes any GPIO pin as an output
///
/// @param gpio: uv_gpios_e pin to be configured
/// @param initial_val: True of false, the initial output value
#define uv_gpio_init_output(gpio, initial_val) \
	uv_gpio_configure(gpio, 0); \
	port(CAT(gpio, _port))->DIR |= (1 << CAT(gpio, _pin)); \
	uv_gpio_set(gpio, initial_val)




/// @brief: Initializes a GPIO input pin with pin change interrupts. Only PORT 0 & 2
/// pins can be configured with this.
///
/// @note: If this macro causes a compile error, the issue is most probably that
/// GPIO port doesn't support interrupt. Only port 0 and 2 support interrupts.
///
/// @param gpio: uv_gpios_e, gpio pin to be configured
/// @param confs: OR'red uv_gpio_input_config_e configuration flags.
/// @param interrupt_confs: OR'red uv_gpio_interrupt_config_e configuration flags.
#define uv_gpio_init_input_int(gpio, confs, interrupt_confs) \
	port(CAT(gpio, _port))->DIR &= ~(1 << CAT(gpio, _pin)); \
	uv_gpio_configure(gpio, confs); \
	CAT(CAT(LPC_GPIOINT->IO, CAT(gpio, _port)), IntEnF) |= ((interrupt_conf & (~1)) << CAT(gpio, _pin)); \
	CAT(CAT(LPC_GPIOINT->IO, CAT(gpio, _port)), IntEnR) |= ((interrupt_conf & (~(1 << 1))) << CAT(gpio, _pin))


/// @brief: Initializes any GPIO pin as an input
///
/// @param gpio: uv_gpios_e, gpio pin to be configured
/// @param confs: OR'red uv_gpio_input_config_e configuration flags.
#define uv_gpio_init_input(gpio, confs) \
	port(CAT(gpio, _port))->DIR &= ~(1 << CAT(gpio, _pin)); \
	uv_gpio_configure(gpio, confs)




/// @brief: Configures the IOCONFIG register with gpio_input_configurations_e values
///
/// @note: Only to be used inside this HAL library!
#define uv_gpio_configure(gpio, input_config)\
	(CAT(gpio, _config(input_config)))





#if CONFIG_TARGET_LPC11C14

/// @brief: Configures the pin's IOCON register with the given configurations
void uv_gpio_configure(uv_gpios_e gpio, uv_gpio_input_config_e value);

/// @brief: Sets the output value on an output gpio pin
/// @pre: uv_gpio_init_output should have been called for this pin
uv_errors_e uv_gpio_set(uv_gpios_e gpio, bool value);

/// @brief: Toggles the state of the output gpio pin
/// @pre: uv_gpio_init_output should have been called for this pin
uv_errors_e uv_gpio_toggle(uv_gpios_e gpio);

/// @brief: Gets the input from the input gpio pin.
/// @pre: uv_gpio_init_input should have been called on this pin.
bool uv_gpio_get(uv_gpios_e gpio);


/// @brief: Returns the pin's IOCON register
///
/// @note: Only to be used inside this library
volatile uint32_t *__uv_gpio_get_iocon(uv_gpios_e gpio);
#endif

#if CONFIG_TARGET_LPC11C14
enum uv_gpios_e {
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
	PIO3_3 = GPIO_PORT_3 | 3,
#endif

};
#endif



#endif /* UW_GPIO_H_ */
