/*
 * uv_gpio_controller.c
 *
 *  Created on: Nov 16, 2015
 *      Author: usevolt
 */


#include "uv_gpio.h"


#include <stdio.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif
#include "uv_utilities.h"


#define INT_SENSE_MASK	0x1
#define INT_EVENT_MASK	0x6


// callback function
static void (*callback)(void*, uv_gpios_e) = 0;

#if CONFIG_TARGET_LPC11C14
// interrupt handler routine
static void isr(LPC_GPIO_TypeDef *GPIO, uv_gpios_e port);
#endif


#if CONFIG_TARGET_LPC11C14
uv_errors_e uv_gpio_init_output(uv_gpios_e gpio, bool initial_value) {
	uv_gpio_configure(gpio, 0);
	// set pin direction to output
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uv_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
	LPC_GPIO->DIR |= (1 << PIN(gpio));
	uv_gpio_set(gpio, initial_value);

	return uv_err(ERR_NONE);
}


uv_errors_e uv_gpio_init_input(uv_gpios_e gpio, uv_gpio_input_config_e configurations,
		uv_gpio_interrupt_config_e int_configurations) {
	// set pin direction to input
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uv_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
	LPC_GPIO->DIR &= ~(1 << PIN(gpio));
	uv_gpio_configure(gpio, configurations);

#if CONFIG_TARGET_LPC11C14
	if (int_configurations == INT_DISABLE) {
		// disable interrupt
		LPC_GPIO->IE &= ~(1 << PIN(gpio));
	}
	else {
		// clear all pending interrupts on this pin
		LPC_GPIO->IC |= (1 << PIN(gpio));
		// interrupt sense: edge or level sensitive
		LPC_GPIO->IS = (LPC_GPIO->IS & ~(1 << PIN(gpio))) |
		((int_configurations & INT_SENSE_MASK) << PIN(gpio));
		// if pin is configured to trigger interrupt on both edges
		if ((int_configurations & INT_EVENT_MASK) == INT_BOTH_EDGES) {
			LPC_GPIO->IBE |= (1 << PIN(gpio));
		}
		else {
			// select interrupt edge or level
			LPC_GPIO->IEV = (LPC_GPIO->IEV & ~(1 << PIN(gpio))) |
					(((int_configurations & INT_EVENT_MASK) >> 1) << PIN(gpio));
		}
		// enable interrupt
		LPC_GPIO->IE |= (1 << PIN(gpio));
	}
#elif CONFIG_TARGET_LPC1785


	unsigned int port = PORT(gpio);
	// disable interrupts
	if (int_configurations == INT_DISABLE) {
		if (port == GPIO_PORT_0) {
			LPC_GPIOINT->IO0IntEnF &= ~(1 << PIN(gpio));
			LPC_GPIOINT->IO0IntEnR &= ~(1 << PIN(gpio));
		}
		else if (port == GPIO_PORT_2) {
			LPC_GPIOINT->IO2IntEnF &= ~(1 << PIN(gpio));
			LPC_GPIOINT->IO2IntEnR &= ~(1 << PIN(gpio));
		}
	}
	else {
		if (port != GPIO_PORT_0 && port != GPIO_PORT_2) {
			__uv_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_GPIO);
		}
		if (port == GPIO_PORT_0) {
			LPC_GPIOINT->IO0IntClr |= (1 << PIN(gpio));
			if (int_configurations & INT_FALLING_EDGE) {
				LPC_GPIOINT->IO0IntEnF |= (1 << PIN(gpio));
			}
			if (int_configurations & INT_RISING_EDGE) {
				LPC_GPIOINT->IO0IntEnR |= (1 << PIN(gpio));
			}
		}
		else {
			LPC_GPIOINT->IO2IntClr |= (1 << PIN(gpio));
			if (int_configurations & INT_FALLING_EDGE) {
				LPC_GPIOINT->IO2IntEnF |= (1 << PIN(gpio));
			}
			if (int_configurations & INT_RISING_EDGE) {
				LPC_GPIOINT->IO2IntEnR |= (1 << PIN(gpio));
			}
		}
	}
#endif

	return uv_err(ERR_NONE);
}

#endif


#if CONFIG_TARGET_LPC11C14
uv_errors_e uv_gpio_set(uv_gpios_e gpio, bool value) {
	//make sure value is either 1 or 0
	value = value ? 1 : 0;
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uv_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
#if CONFIG_TARGET_LPC11C14
	LPC_GPIO->DATA = (LPC_GPIO->DATA & ~(1 << PIN(gpio))) | (value << PIN(gpio));
#elif defined (CONFIG_TARGET_LPC1785)
	if (value) {
		LPC_GPIO->SET |= (1 << PIN(gpio));
	}
	else {
		LPC_GPIO->CLR |= (1 << PIN(gpio));
	}
#endif

	return uv_err(ERR_NONE);
}

uv_errors_e uv_gpio_toggle(uv_gpios_e gpio) {
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uv_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
#if CONFIG_TARGET_LPC11C14
	LPC_GPIO->DATA ^= (1 << PIN(gpio));
#elif CONFIG_TARGET_LPC1785
	LPC_GPIO->PIN ^= (1 << PIN(gpio));
#endif

	return uv_err(ERR_NONE);
}


bool uv_gpio_get(uv_gpios_e gpio) {
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uv_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
#if CONFIG_TARGET_LPC11C14
	return (bool) ((LPC_GPIO->DATA & (1 << PIN(gpio))) >> PIN(gpio));
#elif CONFIG_TARGET_LPC1785
	return (bool) ((LPC_GPIO->PIN & (1 << PIN(gpio))) >> PIN(gpio));
#endif
}

#endif


void uv_gpio_add_interrupt_callback(void (*callback_function)(void*, uv_gpios_e)) {
	callback = callback_function;
#if CONFIG_TARGET_LPC11C14
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
#elif CONFIG_TARGET_LPC1785
	NVIC_EnableIRQ(GPIO_IRQn);
#endif
}

#if CONFIG_TARGET_LPC11C14

void PIOINT0_IRQHandler (void) {
	isr(LPC_GPIO0, GPIO_PORT_0);
}

void PIOINT1_IRQHandler (void) {
	isr(LPC_GPIO1, GPIO_PORT_1);
}

void PIOINT2_IRQHandler (void) {
	isr(LPC_GPIO2, GPIO_PORT_2);
}

void PIOINT3_IRQHandler (void) {
	isr(LPC_GPIO3, GPIO_PORT_3);
}

static void isr(LPC_GPIO_TypeDef *GPIO, uv_gpios_e port) {
	int i = 0;
	// i should be as big as there is IO's in a port
	while(GPIO->MIS) {
		// loop trough all interrupt bits and call callback function
		// for all pins which caused interrupts
		if (GPIO->MIS & (1 << i)) {
			if (callback) {
				callback(__uv_get_user_ptr(), (uv_gpios_e) (port + i));
				// clear interrupt on this pin only
				GPIO->IC |= (1 << i);
			}
		}
		i++;
	}
	__NOP();
	__NOP();
}

#elif CONFIG_TARGET_LPC1785

void GPIO_IRQHandler(void) {
	int i = 0;
	while(LPC_GPIOINT->IO0IntStatF || LPC_GPIOINT->IO0IntStatR) {
		// loop trough all interrupt bits and call callback function
		// for all pins which caused interrupts
		if (LPC_GPIOINT->IO0IntStatF & (1 << i) || LPC_GPIOINT->IO0IntStatR & (1 << i)) {
			if (callback) {
				callback(__uv_get_user_ptr(), PIO0_0 + i);
				// clear interrupt on this pin only
				LPC_GPIOINT->IO0IntClr |= (1 << i);
			}
		}
		i++;
	}
	i = 0;
	while(LPC_GPIOINT->IO2IntStatF || LPC_GPIOINT->IO2IntStatR) {
		// loop trough all interrupt bits and call callback function
		// for all pins which caused interrupts
		if (LPC_GPIOINT->IO2IntStatF & (1 << i) || LPC_GPIOINT->IO2IntStatR & (1 << i)) {
			if (callback) {
				callback(__uv_get_user_ptr(), PIO2_0 + i);
				// clear interrupt on this pin only
				LPC_GPIOINT->IO2IntClr |= (1 << i);
			}
		}
		i++;
	}
	__NOP();
	__NOP();
}

#endif

#if CONFIG_TARGET_LPC11C14
volatile uint32_t *__uv_gpio_get_iocon(uv_gpios_e gpio) {
	switch (gpio) {
#if (CONFIG_PORT0 | CONFIG_PIO0_0)
		case PIO0_0: return &LPC_IOCON->RESET_PIO0_0;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_1)
		case PIO0_1: return &LPC_IOCON->PIO0_1;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_2)
		case PIO0_2: return &LPC_IOCON->PIO0_2;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_3)
		case PIO0_3: return &LPC_IOCON->PIO0_3;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_4)
		case PIO0_4: return &LPC_IOCON->PIO0_4;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_5)
		case PIO0_5: return &LPC_IOCON->PIO0_5;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_6)
		case PIO0_6: return &LPC_IOCON->PIO0_6;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_7)
		case PIO0_7: return &LPC_IOCON->PIO0_7;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_8)
		case PIO0_8: return &LPC_IOCON->PIO0_8;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_9)
		case PIO0_9: return &LPC_IOCON->PIO0_9;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_10)
		case PIO0_10: return &LPC_IOCON->SWCLK_PIO0_10;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_11)
		case PIO0_11: return &LPC_IOCON->R_PIO0_11;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_0)
		case PIO1_0: return &LPC_IOCON->R_PIO1_0;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_1)
		case PIO1_1: return &LPC_IOCON->R_PIO1_1;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_2)
		case PIO1_2: return &LPC_IOCON->R_PIO1_2;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_3)
		case PIO1_3: return &LPC_IOCON->SWDIO_PIO1_3;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_4)
		case PIO1_4: return &LPC_IOCON->PIO1_4;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_5)
		case PIO1_5: return &LPC_IOCON->PIO1_5;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_6)
		case PIO1_6: return &LPC_IOCON->PIO1_6;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_7)
		case PIO1_7: return &LPC_IOCON->PIO1_7;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_8)
		case PIO1_8: return &LPC_IOCON->PIO1_8;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_9)
		case PIO1_9: return &LPC_IOCON->PIO1_9;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_10)
		case PIO1_10: return &LPC_IOCON->PIO1_10;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_11)
		case PIO1_11: return &LPC_IOCON->PIO1_11;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_0)
		case PIO2_0: return &LPC_IOCON->PIO2_0;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_1)
		case PIO2_1: return &LPC_IOCON->PIO2_1;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_2)
		case PIO2_2: return &LPC_IOCON->PIO2_2;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_3)
		case PIO2_3: return &LPC_IOCON->PIO2_3;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_4)
		case PIO2_4: return &LPC_IOCON->PIO2_4;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_5)
		case PIO2_5: return &LPC_IOCON->PIO2_5;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_6)
		case PIO2_6: return &LPC_IOCON->PIO2_6;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_7)
		case PIO2_7: return &LPC_IOCON->PIO2_7;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_8)
		case PIO2_8: return &LPC_IOCON->PIO2_8;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_9)
		case PIO2_9: return &LPC_IOCON->PIO2_9;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_10)
		case PIO2_10: return &LPC_IOCON->PIO2_10;
#endif
#if (CONFIG_PORT2 | CONFIG_PIO2_11)
		case PIO2_11: return &LPC_IOCON->PIO2_11;
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_0)
		case PIO3_0: return &LPC_IOCON->PIO3_0;
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_1)
		case PIO3_1: return &LPC_IOCON->PIO3_1;
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_2)
		case PIO3_2: return &LPC_IOCON->PIO3_2;
#endif
#if (CONFIG_PORT3 | CONFIG_PIO3_3)
		case PIO3_3: return &LPC_IOCON->PIO3_3;
#endif
		default: return NULL;
	}
}
#endif


#if CONFIG_TARGET_LPC11C14
void uv_gpio_configure(uv_gpios_e gpio, uv_gpio_input_config_e value) {
	switch (gpio) {
#if (CONFIG_PORT0 | CONFIG_PIO0_0)
		case PIO0_0:
			*__uv_gpio_get_iocon(gpio) = value | 1; break;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_10)
		case PIO0_10:
			*__uv_gpio_get_iocon(gpio) = value | 1; break;
#endif
#if (CONFIG_PORT0 | CONFIG_PIO0_11)
		case PIO0_11:
			*__uv_gpio_get_iocon(gpio) = value | (1 + (1 << 7)); break;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_0)
		case PIO1_0:
			*__uv_gpio_get_iocon(gpio) = value | (1 + (1 << 7)); break;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_1)
		case PIO1_1:
			*__uv_gpio_get_iocon(gpio) = value | (1 + (1 << 7)); break;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_2)
		case PIO1_2:
			*__uv_gpio_get_iocon(gpio) = value | (1 + (1 << 7)); break;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_3)
		case PIO1_3:
			*__uv_gpio_get_iocon(gpio) = value | (1 + (1 << 7)); break;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_4)
		case PIO1_4:
			*__uv_gpio_get_iocon(gpio) = value | (1 << 7); break;
#endif
#if (CONFIG_PORT1 | CONFIG_PIO1_11)
		case PIO1_11:
			*__uv_gpio_get_iocon(gpio) = value | (1 << 7); break;
#endif
		default: *__uv_gpio_get_iocon(gpio) = value; break;
	}
}


static LPC_GPIO_TypeDef* get_port(uv_gpios_e gpio) {
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		return LPC_GPIO0;
	case GPIO_PORT_1:
		return LPC_GPIO1;
	case GPIO_PORT_2:
		return LPC_GPIO2;
	case GPIO_PORT_3:
		return LPC_GPIO3;
#if CONFIG_TARGET_LPC1785
	case GPIO_PORT_4:
		return LPC_GPIO4;
	case GPIO_PORT_5:
		return LPC_GPIO5;
#endif
	default:
		return NULL;
	}
}

#endif
