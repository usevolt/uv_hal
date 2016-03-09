/*
 * uw_gpio_controller.c
 *
 *  Created on: Nov 16, 2015
 *      Author: usevolt
 */


#include <stdio.h>
#include "uw_gpio.h"
#ifdef LPC11C14
#include "LPC11xx.h"
#elif defined(LPC1785)
#include "LPC177x_8x.h"
#endif
#include "uw_utilities.h"


#define INT_SENSE_MASK	0x1
#define INT_EVENT_MASK	0x6


/// @brief: Set's the IOCON value for the pin gpio
static void set_iocon(uw_gpios_e gpio, uw_gpio_input_config_e value);

/// @brief: Returns the GPIO's port
static LPC_GPIO_TypeDef* get_port(uw_gpios_e gpio);

// callback function
static void (*callback)(void*, uw_gpios_e) = 0;

#ifdef LPC11C14
// interrupt handler routine
static void isr(LPC_GPIO_TypeDef *GPIO, uw_gpios_e port);
#endif

uw_errors_e uw_gpio_init_output(uw_gpios_e gpio, bool initial_value) {
	set_iocon(gpio, 0);
	// set pin direction to output
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
	LPC_GPIO->DIR |= (1 << PIN(gpio));
	uw_gpio_set_pin(gpio, initial_value);

	return ERR_NONE;
}


uw_errors_e uw_gpio_init_input(uw_gpios_e gpio, uw_gpio_input_config_e configurations,
		uw_gpio_interrupt_config_e int_configurations) {
	// set pin direction to input
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
	LPC_GPIO->DIR &= ~(1 << PIN(gpio));
	set_iocon(gpio, configurations);

#ifdef LPC11C14
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
#elif defined(LPC1785)


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
			__uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_GPIO);
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

	return ERR_NONE;
}

uw_errors_e uw_gpio_set_pin(uw_gpios_e gpio, bool value) {
	//make sure value is either 1 or 0
	value = value ? 1 : 0;
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
#ifdef LPC11C14
	LPC_GPIO->DATA = (LPC_GPIO->DATA & ~(1 << PIN(gpio))) | (value << PIN(gpio));
#elif defined (LPC1785)
	if (value) {
		LPC_GPIO->SET |= (1 << PIN(gpio));
	}
	else {
		LPC_GPIO->CLR |= (1 << PIN(gpio));
	}
#endif

	return ERR_NONE;
}

uw_errors_e uw_gpio_toggle_pin(uw_gpios_e gpio) {
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
#ifdef LPC11C14
	LPC_GPIO->DATA ^= (1 << PIN(gpio));
#elif defined(LPC1785)
	LPC_GPIO->PIN ^= (1 << PIN(gpio));
#endif

	return ERR_NONE;
}


bool uw_gpio_get_pin(uw_gpios_e gpio) {
	LPC_GPIO_TypeDef* LPC_GPIO = get_port(gpio);
	if (!LPC_GPIO) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_GPIO);
	}
#ifdef LPC11C14
	return (bool) ((LPC_GPIO->DATA & (1 << PIN(gpio))) >> PIN(gpio));
#elif defined(LPC1785)
	return (bool) ((LPC_GPIO->PIN & (1 << PIN(gpio))) >> PIN(gpio));
#endif
}



void uw_gpio_add_interrupt_callback(void (*callback_function)(void*, uw_gpios_e)) {
	callback = callback_function;
#ifdef LPC11C14
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
#elif defined(LPC1785)
	NVIC_EnableIRQ(GPIO_IRQn);
#endif
}

#ifdef LPC11C14

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

static void isr(LPC_GPIO_TypeDef *GPIO, uw_gpios_e port) {
	int i = 0;
	// i should be as big as there is IO's in a port
	while(GPIO->MIS) {
		// loop trough all interrupt bits and call callback function
		// for all pins which caused interrupts
		if (GPIO->MIS & (1 << i)) {
			if (callback) {
				callback(__uw_get_user_ptr(), (uw_gpios_e) (port + i));
				// clear interrupt on this pin only
				GPIO->IC |= (1 << i);
			}
		}
		i++;
	}
	__NOP();
	__NOP();
}

#elif defined(LPC1785)

void GPIO_IRQHandler(void) {
	int i = 0;
	while(LPC_GPIOINT->IO0IntStatF || LPC_GPIOINT->IO0IntStatR) {
		// loop trough all interrupt bits and call callback function
		// for all pins which caused interrupts
		if (LPC_GPIOINT->IO0IntStatF & (1 << i) || LPC_GPIOINT->IO0IntStatR & (1 << i)) {
			if (callback) {
				callback(__uw_get_user_ptr(), (uw_gpios_e) GPIO_PORT_0 + i);
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
				callback(__uw_get_user_ptr(), (uw_gpios_e) GPIO_PORT_2 + i);
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

volatile uint32_t *__uw_gpio_get_iocon(uw_gpios_e gpio) {
#ifdef LPC11C14
	switch (gpio) {
		case PIO0_0: return &LPC_IOCON->RESET_PIO0_0;
		case PIO0_1: return &LPC_IOCON->PIO0_1;
		case PIO0_2: return &LPC_IOCON->PIO0_2;
		case PIO0_3: return &LPC_IOCON->PIO0_3;
		case PIO0_4: return &LPC_IOCON->PIO0_4;
		case PIO0_5: return &LPC_IOCON->PIO0_5;
		case PIO0_6: return &LPC_IOCON->PIO0_6;
		case PIO0_7: return &LPC_IOCON->PIO0_7;
		case PIO0_8: return &LPC_IOCON->PIO0_8;
		case PIO0_9: return &LPC_IOCON->PIO0_9;
		case PIO0_10: return &LPC_IOCON->SWCLK_PIO0_10;
		case PIO0_11: return &LPC_IOCON->R_PIO0_11;
		case PIO1_0: return &LPC_IOCON->R_PIO1_0;
		case PIO1_1: return &LPC_IOCON->R_PIO1_1;
		case PIO1_2: return &LPC_IOCON->R_PIO1_2;
		case PIO1_3: return &LPC_IOCON->SWDIO_PIO1_3;
		case PIO1_4: return &LPC_IOCON->PIO1_4;
		case PIO1_5: return &LPC_IOCON->PIO1_5;
		case PIO1_6: return &LPC_IOCON->PIO1_6;
		case PIO1_7: return &LPC_IOCON->PIO1_7;
		case PIO1_8: return &LPC_IOCON->PIO1_8;
		case PIO1_9: return &LPC_IOCON->PIO1_9;
		case PIO1_10: return &LPC_IOCON->PIO1_10;
		case PIO1_11: return &LPC_IOCON->PIO1_11;
		case PIO2_0: return &LPC_IOCON->PIO2_0;
		case PIO2_1: return &LPC_IOCON->PIO2_1;
		case PIO2_2: return &LPC_IOCON->PIO2_2;
		case PIO2_3: return &LPC_IOCON->PIO2_3;
		case PIO2_4: return &LPC_IOCON->PIO2_4;
		case PIO2_5: return &LPC_IOCON->PIO2_5;
		case PIO2_6: return &LPC_IOCON->PIO2_6;
		case PIO2_7: return &LPC_IOCON->PIO2_7;
		case PIO2_8: return &LPC_IOCON->PIO2_8;
		case PIO2_9: return &LPC_IOCON->PIO2_9;
		case PIO2_10: return &LPC_IOCON->PIO2_10;
		case PIO2_11: return &LPC_IOCON->PIO2_11;
		case PIO3_0: return &LPC_IOCON->PIO3_0;
		case PIO3_1: return &LPC_IOCON->PIO3_1;
		case PIO3_2: return &LPC_IOCON->PIO3_2;
		case PIO3_3: return &LPC_IOCON->PIO3_3;
		default: return NULL;
	}
#elif defined(LPC1785)
	switch (gpio) {
	case PIO0_0: return &LPC_IOCON->P0_0;
	case PIO0_1: return &LPC_IOCON->P0_1;
	case PIO0_2: return &LPC_IOCON->P0_2;
	case PIO0_3: return &LPC_IOCON->P0_3;
	case PIO0_4: return &LPC_IOCON->P0_4;
	case PIO0_5: return &LPC_IOCON->P0_5;
	case PIO0_6: return &LPC_IOCON->P0_6;
	case PIO0_7: return &LPC_IOCON->P0_7;
	case PIO0_8: return &LPC_IOCON->P0_8;
	case PIO0_9: return &LPC_IOCON->P0_9;
	case PIO0_10: return &LPC_IOCON->P0_10;
	case PIO0_11: return &LPC_IOCON->P0_11;
	case PIO0_12: return &LPC_IOCON->P0_12;
	case PIO0_13: return &LPC_IOCON->P0_13;
	case PIO0_14: return &LPC_IOCON->P0_14;
	case PIO0_15: return &LPC_IOCON->P0_15;
	case PIO0_16: return &LPC_IOCON->P0_16;
	case PIO0_17: return &LPC_IOCON->P0_17;
	case PIO0_18: return &LPC_IOCON->P0_18;
	case PIO0_19: return &LPC_IOCON->P0_19;
	case PIO0_20: return &LPC_IOCON->P0_20;
	case PIO0_21: return &LPC_IOCON->P0_21;
	case PIO0_22: return &LPC_IOCON->P0_22;
	case PIO0_23: return &LPC_IOCON->P0_23;
	case PIO0_24: return &LPC_IOCON->P0_24;
	case PIO0_25: return &LPC_IOCON->P0_25;
	case PIO0_26: return &LPC_IOCON->P0_26;
	case PIO0_27: return &LPC_IOCON->P0_27;
	case PIO0_28: return &LPC_IOCON->P0_28;
	case PIO0_29: return &LPC_IOCON->P0_29;
	case PIO0_30: return &LPC_IOCON->P0_30;
	case PIO0_31: return &LPC_IOCON->P0_31;
	case PIO1_0: return &LPC_IOCON->P1_0;
	case PIO1_1: return &LPC_IOCON->P1_1;
	case PIO1_2: return &LPC_IOCON->P1_2;
	case PIO1_3: return &LPC_IOCON->P1_3;
	case PIO1_4: return &LPC_IOCON->P1_4;
	case PIO1_5: return &LPC_IOCON->P1_5;
	case PIO1_6: return &LPC_IOCON->P1_6;
	case PIO1_7: return &LPC_IOCON->P1_7;
	case PIO1_8: return &LPC_IOCON->P1_8;
	case PIO1_9: return &LPC_IOCON->P1_9;
	case PIO1_10: return &LPC_IOCON->P1_10;
	case PIO1_11: return &LPC_IOCON->P1_11;
	case PIO1_12: return &LPC_IOCON->P1_12;
	case PIO1_13: return &LPC_IOCON->P1_13;
	case PIO1_14: return &LPC_IOCON->P1_14;
	case PIO1_15: return &LPC_IOCON->P1_15;
	case PIO1_16: return &LPC_IOCON->P1_16;
	case PIO1_17: return &LPC_IOCON->P1_17;
	case PIO1_18: return &LPC_IOCON->P1_18;
	case PIO1_19: return &LPC_IOCON->P1_19;
	case PIO1_20: return &LPC_IOCON->P1_20;
	case PIO1_21: return &LPC_IOCON->P1_21;
	case PIO1_22: return &LPC_IOCON->P1_22;
	case PIO1_23: return &LPC_IOCON->P1_23;
	case PIO1_24: return &LPC_IOCON->P1_24;
	case PIO1_25: return &LPC_IOCON->P1_25;
	case PIO1_26: return &LPC_IOCON->P1_26;
	case PIO1_27: return &LPC_IOCON->P1_27;
	case PIO1_28: return &LPC_IOCON->P1_28;
	case PIO1_29: return &LPC_IOCON->P1_29;
	case PIO1_30: return &LPC_IOCON->P1_30;
	case PIO1_31: return &LPC_IOCON->P1_31;
	case PIO2_0: return &LPC_IOCON->P2_0;
	case PIO2_1: return &LPC_IOCON->P2_1;
	case PIO2_2: return &LPC_IOCON->P2_2;
	case PIO2_3: return &LPC_IOCON->P2_3;
	case PIO2_4: return &LPC_IOCON->P2_4;
	case PIO2_5: return &LPC_IOCON->P2_5;
	case PIO2_6: return &LPC_IOCON->P2_6;
	case PIO2_7: return &LPC_IOCON->P2_7;
	case PIO2_8: return &LPC_IOCON->P2_8;
	case PIO2_9: return &LPC_IOCON->P2_9;
	case PIO2_10: return &LPC_IOCON->P2_10;
	case PIO2_11: return &LPC_IOCON->P2_11;
	case PIO2_12: return &LPC_IOCON->P2_12;
	case PIO2_13: return &LPC_IOCON->P2_13;
	case PIO2_14: return &LPC_IOCON->P2_14;
	case PIO2_15: return &LPC_IOCON->P2_15;
	case PIO2_16: return &LPC_IOCON->P2_16;
	case PIO2_17: return &LPC_IOCON->P2_17;
	case PIO2_18: return &LPC_IOCON->P2_18;
	case PIO2_19: return &LPC_IOCON->P2_19;
	case PIO2_20: return &LPC_IOCON->P2_20;
	case PIO2_21: return &LPC_IOCON->P2_21;
	case PIO2_22: return &LPC_IOCON->P2_22;
	case PIO2_23: return &LPC_IOCON->P2_23;
	case PIO2_24: return &LPC_IOCON->P2_24;
	case PIO2_25: return &LPC_IOCON->P2_25;
	case PIO2_26: return &LPC_IOCON->P2_26;
	case PIO2_27: return &LPC_IOCON->P2_27;
	case PIO2_28: return &LPC_IOCON->P2_28;
	case PIO2_29: return &LPC_IOCON->P2_29;
	case PIO2_30: return &LPC_IOCON->P2_30;
	case PIO2_31: return &LPC_IOCON->P2_31;
	case PIO3_0: return &LPC_IOCON->P3_0;
	case PIO3_1: return &LPC_IOCON->P3_1;
	case PIO3_2: return &LPC_IOCON->P3_2;
	case PIO3_3: return &LPC_IOCON->P3_3;
	case PIO3_4: return &LPC_IOCON->P3_4;
	case PIO3_5: return &LPC_IOCON->P3_5;
	case PIO3_6: return &LPC_IOCON->P3_6;
	case PIO3_7: return &LPC_IOCON->P3_7;
	case PIO3_8: return &LPC_IOCON->P3_8;
	case PIO3_9: return &LPC_IOCON->P3_9;
	case PIO3_10: return &LPC_IOCON->P3_10;
	case PIO3_11: return &LPC_IOCON->P3_11;
	case PIO3_12: return &LPC_IOCON->P3_12;
	case PIO3_13: return &LPC_IOCON->P3_13;
	case PIO3_14: return &LPC_IOCON->P3_14;
	case PIO3_15: return &LPC_IOCON->P3_15;
	case PIO3_16: return &LPC_IOCON->P3_16;
	case PIO3_17: return &LPC_IOCON->P3_17;
	case PIO3_18: return &LPC_IOCON->P3_18;
	case PIO3_19: return &LPC_IOCON->P3_19;
	case PIO3_20: return &LPC_IOCON->P3_20;
	case PIO3_21: return &LPC_IOCON->P3_21;
	case PIO3_22: return &LPC_IOCON->P3_22;
	case PIO3_23: return &LPC_IOCON->P3_23;
	case PIO3_24: return &LPC_IOCON->P3_24;
	case PIO3_25: return &LPC_IOCON->P3_25;
	case PIO3_26: return &LPC_IOCON->P3_26;
	case PIO3_27: return &LPC_IOCON->P3_27;
	case PIO3_28: return &LPC_IOCON->P3_28;
	case PIO3_29: return &LPC_IOCON->P3_29;
	case PIO3_30: return &LPC_IOCON->P3_30;
	case PIO3_31: return &LPC_IOCON->P3_31;
	case PIO4_0: return &LPC_IOCON->P4_0;
	case PIO4_1: return &LPC_IOCON->P4_1;
	case PIO4_2: return &LPC_IOCON->P4_2;
	case PIO4_3: return &LPC_IOCON->P4_3;
	case PIO4_4: return &LPC_IOCON->P4_4;
	case PIO4_5: return &LPC_IOCON->P4_5;
	case PIO4_6: return &LPC_IOCON->P4_6;
	case PIO4_7: return &LPC_IOCON->P4_7;
	case PIO4_8: return &LPC_IOCON->P4_8;
	case PIO4_9: return &LPC_IOCON->P4_9;
	case PIO4_10: return &LPC_IOCON->P4_10;
	case PIO4_11: return &LPC_IOCON->P4_11;
	case PIO4_12: return &LPC_IOCON->P4_12;
	case PIO4_13: return &LPC_IOCON->P4_13;
	case PIO4_14: return &LPC_IOCON->P4_14;
	case PIO4_15: return &LPC_IOCON->P4_15;
	case PIO4_16: return &LPC_IOCON->P4_16;
	case PIO4_17: return &LPC_IOCON->P4_17;
	case PIO4_18: return &LPC_IOCON->P4_18;
	case PIO4_19: return &LPC_IOCON->P4_19;
	case PIO4_20: return &LPC_IOCON->P4_20;
	case PIO4_21: return &LPC_IOCON->P4_21;
	case PIO4_22: return &LPC_IOCON->P4_22;
	case PIO4_23: return &LPC_IOCON->P4_23;
	case PIO4_24: return &LPC_IOCON->P4_24;
	case PIO4_25: return &LPC_IOCON->P4_25;
	case PIO4_26: return &LPC_IOCON->P4_26;
	case PIO4_27: return &LPC_IOCON->P4_27;
	case PIO4_28: return &LPC_IOCON->P4_28;
	case PIO4_29: return &LPC_IOCON->P4_29;
	case PIO4_30: return &LPC_IOCON->P4_30;
	case PIO4_31: return &LPC_IOCON->P4_31;
	case PIO5_0: return &LPC_IOCON->P5_0;
	case PIO5_1: return &LPC_IOCON->P5_1;
	case PIO5_2: return &LPC_IOCON->P5_2;
	case PIO5_3: return &LPC_IOCON->P5_3;
	case PIO5_4: return &LPC_IOCON->P5_4;
	default: return NULL;
	}
#endif
}


static void set_iocon(uw_gpios_e gpio, uw_gpio_input_config_e value) {
#ifdef LPC11C14
	switch (gpio) {
		case PIO0_0:
		case PIO0_10: *__uw_gpio_get_iocon(gpio) = value | 1; break;
		case PIO0_11:
		case PIO1_0:
		case PIO1_1:
		case PIO1_2:
		case PIO1_3: *__uw_gpio_get_iocon(gpio) = value | (1 + (1 << 7)); break;
		case PIO1_4:
		case PIO1_11: *__uw_gpio_get_iocon(gpio) = value | (1 << 7); break;
		default: *__uw_gpio_get_iocon(gpio) = value; break;
	}
#elif defined(LPC1785)
	/* Refer to the LPC1785 manual for info about these tables */
	// glitch filter is not supported
	uw_gpio_input_config_e table81 = value & (~(GLITCH_FILTER_ENABLED));
	// hysteresis is not supported
	uw_gpio_input_config_e table83 = value & (~(HYSTERESIS_ENABLED));
	// digital mode
	table83 |= (1 << 7);
	// glitch filter enable needs to be toggled to function correctly
	table83 ^= GLITCH_FILTER_ENABLED;
	// usb pins: Only basic GPIO available without any funny configurations
	uw_gpio_input_config_e table85 = 0;
	// I2C modes not set in GPIO config
	uw_gpio_input_config_e table87 = value & (INVERSE_POLARITY);
	// table 89 has all available configurations
	// bit 7 has to be 1 for normal operation
	uw_gpio_input_config_e table89 = value | (1 << 7);

	switch (gpio) {
	case PIO0_12:
	case PIO0_13:
	case PIO0_23:
	case PIO0_24:
	case PIO0_25:
	case PIO0_26:
	case PIO1_30:
	case PIO1_31: *__uw_gpio_get_iocon(gpio) = table83; break;
	case PIO0_29:
	case PIO0_30:
	case PIO0_31: *__uw_gpio_get_iocon(gpio) = table85; break;
	case PIO0_27:
	case PIO0_28:
	case PIO5_2:
	case PIO5_3: *__uw_gpio_get_iocon(gpio) = table87; break;
	case PIO0_7:
	case PIO0_8:
	case PIO0_9:
	case PIO1_5:
	case PIO1_6:
	case PIO1_7:
	case PIO1_14:
	case PIO1_16:
	case PIO1_17: *__uw_gpio_get_iocon(gpio) = table89; break;
	default: *__uw_gpio_get_iocon(gpio) = table81; break;
	}
#endif
}
static LPC_GPIO_TypeDef* get_port(uw_gpios_e gpio) {
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		return LPC_GPIO0;
	case GPIO_PORT_1:
		return LPC_GPIO1;
	case GPIO_PORT_2:
		return LPC_GPIO2;
	case GPIO_PORT_3:
		return LPC_GPIO3;
#ifdef LPC1785
	case GPIO_PORT_4:
		return LPC_GPIO4;
	case GPIO_PORT_5:
		return LPC_GPIO5;
#endif
	default:
		return NULL;
	}
}
