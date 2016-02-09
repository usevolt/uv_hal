/*
 * hal_gpio_controller.c
 *
 *  Created on: Nov 16, 2015
 *      Author: usevolt
 */


#include "hal_gpio_controller.h"
#include <stdio.h>
#include "LPC11xx.h"



#define INT_SENSE_MASK	0x1
#define INT_EVENT_MASK	0x6

/// @brief: Set's the IOCON value for the pin gpio
static void set_iocon(hal_gpios_e gpio, unsigned int value);

// callback function
static void (*callback)(hal_gpios_e) = 0;


void hal_gpio_init_output(hal_gpios_e gpio, bool initial_value) {
	set_iocon(gpio, 0);
	// set pin direction to output
	LPC_GPIO_TypeDef* LPC_GPIO;
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		LPC_GPIO = LPC_GPIO0;
		break;
	case GPIO_PORT_1:
		LPC_GPIO = LPC_GPIO1;
		break;
	case GPIO_PORT_2:
		LPC_GPIO = LPC_GPIO2;
		break;
	case GPIO_PORT_3:
		LPC_GPIO = LPC_GPIO3;
		break;
	default:
		return;
	}
	LPC_GPIO->DIR |= (1 << PIN(gpio));
	hal_gpio_set_pin(gpio, initial_value);
}



void hal_gpio_init_input(hal_gpios_e gpio, hal_gpio_input_config_e configurations,
		hal_gpio_interrupt_config_e int_configurations) {
	// set pin direction to input
	LPC_GPIO_TypeDef* LPC_GPIO;
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		LPC_GPIO = LPC_GPIO0;
		break;
	case GPIO_PORT_1:
		LPC_GPIO = LPC_GPIO1;
		break;
	case GPIO_PORT_2:
		LPC_GPIO = LPC_GPIO2;
		break;
	case GPIO_PORT_3:
		LPC_GPIO = LPC_GPIO3;
		break;
	default:
		return;
	}
	LPC_GPIO->DIR &= ~(1 << PIN(gpio));
	set_iocon(gpio, configurations);

	if (int_configurations == INT_DISABLE) {
		// disable interrupt
		LPC_GPIO->IE &= ~(1 << PIN(gpio));
	}
	else {
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
}


void hal_gpio_set_pin(hal_gpios_e gpio, bool value) {
	//make sure value is either 1 or 0
	value = value ? 1 : 0;
	LPC_GPIO_TypeDef* LPC_GPIO;
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		LPC_GPIO = LPC_GPIO0;
		break;
	case GPIO_PORT_1:
		LPC_GPIO = LPC_GPIO1;
		break;
	case GPIO_PORT_2:
		LPC_GPIO = LPC_GPIO2;
		break;
	case GPIO_PORT_3:
		LPC_GPIO = LPC_GPIO3;
		break;
	default:
		return;
	}
	LPC_GPIO->DATA = (LPC_GPIO->DATA & ~(1 << PIN(gpio))) | (value << PIN(gpio));
}

void hal_gpio_toggle_pin(hal_gpios_e gpio) {
	LPC_GPIO_TypeDef* LPC_GPIO;
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		LPC_GPIO = LPC_GPIO0;
		break;
	case GPIO_PORT_1:
		LPC_GPIO = LPC_GPIO1;
		break;
	case GPIO_PORT_2:
		LPC_GPIO = LPC_GPIO2;
		break;
	case GPIO_PORT_3:
		LPC_GPIO = LPC_GPIO3;
		break;
	default:
		return;
	}
	LPC_GPIO->DATA ^= (1 << PIN(gpio));

}


bool hal_gpio_get_pin(hal_gpios_e gpio) {
	LPC_GPIO_TypeDef* LPC_GPIO;
	switch (gpio & GPIO_PORT_MASK) {
	case GPIO_PORT_0:
		LPC_GPIO = LPC_GPIO0;
		break;
	case GPIO_PORT_1:
		LPC_GPIO = LPC_GPIO1;
		break;
	case GPIO_PORT_2:
		LPC_GPIO = LPC_GPIO2;
		break;
	case GPIO_PORT_3:
		LPC_GPIO = LPC_GPIO3;
		break;
	default:
		return (bool) 0;
	}
	return (bool) ((LPC_GPIO->DATA & (1 << PIN(gpio))) >> PIN(gpio));
}



void hal_gpio_set_interrupt_callback(void (*callback_function)(hal_gpios_e)) {
	callback = callback_function;
	NVIC_EnableIRQ(EINT0_IRQn);
	NVIC_EnableIRQ(EINT1_IRQn);
	NVIC_EnableIRQ(EINT2_IRQn);
	NVIC_EnableIRQ(EINT3_IRQn);
}


void PIOINT0_IRQHandler (void) {
	int i;
	// i should be as big as there is IO's in a port
	for (i = 0; i < 12; i++) {
		// loop trough all interrupt bits and call callback function for all pins which caused interrupts
		if (((LPC_GPIO0->MIS & (1 << i)) >> i)) {
			if (callback) {
				callback((hal_gpios_e) (GPIO_PORT_0 + i));
				// clear interrupt on this pin only
				LPC_GPIO0->IC |= (1 << i);
			}
		}
	}
	__NOP();
	__NOP();
}

void PIOINT1_IRQHandler (void) {
	int i;
	// i should be as big as there is IO's in a port
	for (i = 0; i < 12; i++) {
		// loop trough all interrupt bits and call callback function for all pins which caused interrupts
		if ((LPC_GPIO1->MIS & (1 << i)) >> i) {
			if (callback) {
				callback((hal_gpios_e) (GPIO_PORT_1 + i));
				// clear interrupt on this pin only
				LPC_GPIO1->IC |= (1 << i);
			}
		}
	}
	__NOP();
	__NOP();
}

void PIOINT2_IRQHandler (void) {
	int i;
	// i should be as big as there is IO's in a port
	for (i = 0; i < 12; i++) {
		// loop trough all interrupt bits and call callback function for all pins which caused interrupts
		if ((LPC_GPIO2->MIS & (1 << i)) >> i) {
			if (callback) {
				callback((hal_gpios_e) (GPIO_PORT_2 + i));
				// clear interrupt on this pin only
				LPC_GPIO2->IC |= (1 << i);
			}
		}
	}
	__NOP();
	__NOP();
}

void PIOINT3_IRQHandler (void) {
	int i;
	// i should be as big as there is IO's in a port
	for (i = 0; i < 4; i++) {
		// loop trough all interrupt bits and call callback function for all pins which caused interrupts
		if ((LPC_GPIO3->MIS & (1 << i)) >> i) {
			if (callback) {
				callback((hal_gpios_e) (GPIO_PORT_3 + i));
				// clear interrupt on this pin only
				LPC_GPIO3->IC |= (1 << i);
			}
		}
	}
	__NOP();
	__NOP();
}

static void set_iocon(hal_gpios_e gpio, unsigned int value) {
	switch (gpio) {
		case PIO0_0:
			LPC_IOCON->RESET_PIO0_0 = value | 1;
			break;
		case PIO0_1:
			LPC_IOCON->PIO0_1 = value;
			break;
		case PIO0_2:
			LPC_IOCON->PIO0_2 = value;
			break;
		case PIO0_3:
			LPC_IOCON->PIO0_3 = value;
			break;
		case PIO0_4:
			LPC_IOCON->PIO0_4 = value;
			break;
		case PIO0_5:
			LPC_IOCON->PIO0_5 = value;
			break;
		case PIO0_6:
			LPC_IOCON->PIO0_6 = value;
			break;
		case PIO0_7:
			LPC_IOCON->PIO0_7 = value;
			break;
		case PIO0_8:
			LPC_IOCON->PIO0_8 = value;
			break;
		case PIO0_9:
			LPC_IOCON->PIO0_9 = value;
			break;
		case PIO0_10:
			LPC_IOCON->SWCLK_PIO0_10 = value | 1;
			break;
		case PIO0_11:
			LPC_IOCON->R_PIO0_11 = value | (1 + (1 << 7));
			break;
		case PIO1_0:
			LPC_IOCON->R_PIO1_0 = value | (1 + (1 << 7));
			break;
		case PIO1_1:
			LPC_IOCON->R_PIO1_1 = value | (1 + (1 << 7));
			break;
		case PIO1_2:
			LPC_IOCON->R_PIO1_2 = value | (1 + (1 << 7));
			break;
		case PIO1_3:
			LPC_IOCON->SWDIO_PIO1_3 = value | (1 + (1 << 7));
			break;
		case PIO1_4:
			LPC_IOCON->PIO1_4 = value | (1 << 7);
			break;
		case PIO1_5:
			LPC_IOCON->PIO1_5 = value;
			break;
		case PIO1_6:
			LPC_IOCON->PIO1_6 = value;
			break;
		case PIO1_7:
			LPC_IOCON->PIO1_7 = value;
			break;
		case PIO1_8:
			LPC_IOCON->PIO1_8 = value;
			break;
		case PIO1_9:
			LPC_IOCON->PIO1_9 = value;
			break;
		case PIO1_10:
			LPC_IOCON->PIO1_10 = value;
			break;
		case PIO1_11:
			LPC_IOCON->PIO1_11 = value | (1 << 7);
			break;
		case PIO2_0:
			LPC_IOCON->PIO2_0 = value;
			break;
		case PIO2_1:
			LPC_IOCON->PIO2_1 = value;
			break;
		case PIO2_2:
			LPC_IOCON->PIO2_2 = value;
			break;
		case PIO2_3:
			LPC_IOCON->PIO2_3 = value;
			break;
		case PIO2_4:
			LPC_IOCON->PIO2_4 = value;
			break;
		case PIO2_5:
			LPC_IOCON->PIO2_5 = value;
			break;
		case PIO2_6:
			LPC_IOCON->PIO2_6 = value;
			break;
		case PIO2_7:
			LPC_IOCON->PIO2_7 = value;
			break;
		case PIO2_8:
			LPC_IOCON->PIO2_8 = value;
			break;
		case PIO2_9:
			LPC_IOCON->PIO2_9 = value;
			break;
		case PIO2_10:
			LPC_IOCON->PIO2_10 = value;
			break;
		case PIO2_11:
			LPC_IOCON->PIO2_11 = value;
			break;
		case PIO3_0:
			LPC_IOCON->PIO3_0 = value;
			break;
		case PIO3_1:
			LPC_IOCON->PIO3_1 = value;
			break;
		case PIO3_2:
			LPC_IOCON->PIO3_2 = value;
			break;
		case PIO3_3:
			LPC_IOCON->PIO3_3 = value;
			break;
		default:
			break;
	}
}
