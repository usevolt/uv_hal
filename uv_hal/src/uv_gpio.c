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



// callback function
static void (*callback)(void*, uv_gpios_e) = 0;

#if CONFIG_TARGET_LPC11C14
// interrupt handler routine
static void isr(LPC_GPIO_TypeDef *GPIO, uv_gpios_e port);
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

