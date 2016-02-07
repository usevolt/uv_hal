/*
 * hal_timer_controller.c
 *
 *  Created on: Jan 28, 2015
 *      Author: usenius
 */


#include <stdlib.h>
#include <stdio.h>
#include "hal_timer_controller.h"
#include "LPC11xx.h"

static void (*CT16B0_callback_function)(int) = NULL;
static void (*CT16B0_pwm_callback_function1)(int) = NULL;
static void (*CT16B0_pwm_callback_function2)(int) = NULL;
static void (*CT16B0_capture0_callback)(uint16_t) = NULL;
static volatile uint16_t TMR16B0_match1_value = 0;

//CT16B0 interrupt handler
void TIMER16_0_IRQHandler(void) {
	//check if match 0 interrupt happened
	if (LPC_TMR16B0->IR & (1 << 0)) {
		//clear interrupt flag
		LPC_TMR16B0->IR |= 1 << 0;
		//if callback function was assigned, call it
		if (CT16B0_callback_function) {
			//parameter is counter value
			CT16B0_callback_function(LPC_TMR16B0->TC & 0xFFFF);
		}
		if (CT16B0_pwm_callback_function1) {
			CT16B0_pwm_callback_function1(LPC_TMR16B0->TC & 0xFFFF);
		}
		//reset timer counter register
		LPC_TMR16B0->TC = 0;
	}
	//check match 1 interrupt
	if (LPC_TMR16B0->IR & (1 << 1)) {
		//clear interrupt flag
		LPC_TMR16B0->IR |= 1 << 1;
		//set new match1 value
		LPC_TMR16B0->MR1 = TMR16B0_match1_value;
		//call callback function
		if (CT16B0_pwm_callback_function2) {
			CT16B0_pwm_callback_function2(LPC_TMR16B0->TC & 0xFFFF);
		}
	}
	//check capture 0 interrupt
	if (LPC_TMR16B0->IR & (1 << 4)) {
		//clear interrupt flag
		LPC_TMR16B0->IR |= (1 << 4);
		//call callback function with the captured timer value
		if (CT16B0_capture0_callback) {
			CT16B0_capture0_callback(LPC_TMR16B0->CR0 & 0xFFFF);
		}
	}
}

void hal_init_CT16B0(int freq, int fosc) {
	//enable sys clock to CT16B0 module
	LPC_SYSCON->SYSAHBCLKCTRL |= 1 << 7;
	//set 16 bit prescaler
	int prescaler = fosc / (freq * 0xFFFF);
	if (prescaler == 0) {
		printf("Warning: Tried to initialize TMR16B0 with too big frequency\n\r");
	}
	LPC_TMR16B0->PR = prescaler;
	//enable interrupts
	NVIC_EnableIRQ(TIMER_16_0_IRQn);
	//generate interrupt on match 0
	LPC_TMR16B0->MCR |= 1 << 0;
	//place match 0 on counter full value
	LPC_TMR16B0->MR0 = 0xFFFF;
	//turn timer 0 on
	LPC_TMR16B0->TCR |= 1 << 0;
}


inline int hal_get_CT16B0_value() {
	return LPC_TMR16B0->TC & 0x7FFF;
}


inline void hal_register_task_to_CT16B0(void (*callback_function)(int)) {
	CT16B0_callback_function = callback_function;
}

void hal_register_pwm_CT16B0(void (*callback_function1)(int),
		void (*callback_function2)(int), uint16_t ppt) {

	//set match1 to zero
	LPC_TMR16B0->MR1 = TMR16B0_match1_value;

	//clamp ppt between borders
	if (ppt > 1000) {
		ppt = 1000;
	}
	if (ppt == 0) {
		ppt = 1;
	}
	//set callback function
	CT16B0_pwm_callback_function1 = callback_function1;
	CT16B0_pwm_callback_function2 = callback_function2;
	//enable interrupt on match 1
	LPC_TMR16B0->MCR |= 1 << 3;
	//set match 1 value
	TMR16B0_match1_value = (ppt * 0xFFFF / 1000) & 0xFFFF;
}





void hal_set_CT16B0CAP0_callback( void (*callback_function)(uint16_t),
		hal_timer_capture_edges_e capture_edges) {

	CT16B0_capture0_callback = callback_function;

	//enable capture 0 interrupt
	LPC_TMR16B0->CCR |= (1 << 2);
	//set capture edge
	switch (capture_edges) {
	case TIMER_CAPTURE_RISING_EDGE:
		LPC_TMR16B0->CCR |= (1 << 0);
		break;
	case TIMER_CAPTURE_FALLING_EDGE:
		LPC_TMR16B0->CCR |= (1 << 1);
		break;
	default:
		LPC_TMR16B0->CCR |= 0x3;
		break;
	}
	//set TMR16B0CAP0 pin to capture mode
	LPC_IOCON->PIO0_2 &= ~0x1;
	LPC_IOCON->PIO0_2 |= 0x2;

}




void hal_init_CT16B1(int freq, int fosc) {
	//enable sys clock to CT16B1 module
	LPC_SYSCON->SYSAHBCLKCTRL |= 1 << 8;
	//set 16 bit prescaler
	int prescaler = fosc / (freq * 0xFFFF);
	if (prescaler == 0) {
		printf("Warning: Tried to initialize TMR16B1 with too big frequency\n\r");
	}
	LPC_TMR16B1->PR = prescaler;
	//enable interrupts
	NVIC_EnableIRQ(TIMER_16_1_IRQn);
	//turn timer 1 on
	LPC_TMR16B1->TCR |= 1 << 0;
}


void hal_init_CT16B1_pwm(hal_timer_match_outputs_e pins, int freq, int fosc) {
	// init the timer
	hal_init_CT16B1(freq, fosc);
	LPC_TMR16B1->TCR &= ~(1 << 0);
	// set match 3 to clear the timer
	LPC_TMR16B1->MCR |= (1 << 10);
	// set match 3 to timer max value
	LPC_TMR16B1->MR3 = 0xFFFF;

	//configure each match output with the pwm signal
	if (pins > 0b111) {
		pins &= 0b111;
	}
	if (pins & MATCH_0) {
		LPC_IOCON->PIO1_9 = (LPC_IOCON->PIO1_9 & (~0x7)) | (0x1);
	}
	if (pins & MATCH_1) {
		LPC_IOCON->PIO1_10 = (LPC_IOCON->PIO1_10 & ~(0x7)) | (0x1 | (1 << 7));
	}
	// MATCH2 is not pinned out on 16BTMR1 (although datasheet says otherwise)

	LPC_TMR16B1->PWMC |= pins;

	LPC_TMR16B1->TCR |= 1 << 0;
}


void hal_set_CT16B1_pwm(hal_timer_pwm_channels_e pwm_channel, uint16_t duty_cycle) {
	uint16_t match_value = 0xFFFF - duty_cycle;
	switch (pwm_channel) {
	case PWM_CHANNEL_0:
		LPC_TMR16B1->MR0 = match_value;
		break;
	case PWM_CHANNEL_1:
		LPC_TMR16B1->MR1 = match_value;
		break;
	case PWM_CHANNEL_2:
		LPC_TMR16B1->MR2 = match_value;
		break;
	default:
		return;
	}
}


uint16_t hal_get_CT16B1_value() {
	return LPC_TMR16B1->TC;
}



uint32_t tick_timer_cycle_time_ms;
static void (*callback_function)(uint32_t) = 0;

// tick timer interrupt handler
inline void SysTick_Handler(void) {
	if (callback_function != 0) {
		callback_function(tick_timer_cycle_time_ms);
	}
}



bool hal_init_tick_timer(uint32_t freq, uint32_t fosc) {
	//enable timer interrupts
	NVIC_EnableIRQ(SysTick_IRQn);
	tick_timer_cycle_time_ms = 1000 / freq;
	if (SysTick_Config(fosc / freq)) {
		//error happened
		return false;
	}
	return true;

}





inline void hal_register_tick_timer_task(
		void (*callback_function_ptr)(uint32_t)) {
	callback_function = callback_function_ptr;
}
