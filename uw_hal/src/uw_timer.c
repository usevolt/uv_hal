/*
 * uw_timer_controller.c
 *
 *  Created on: Jan 28, 2015
 *      Author: usenius
 */


#include <stdlib.h>
#include <stdio.h>
#include "uw_timer.h"
#include "uw_utilities.h"
#ifdef LPC11C14
#include "LPC11xx.h"
#elif defined(LPC1785)

#endif

typedef struct {
#ifdef LPC11C14
	/// @brief: All timer pointers on this hardware for easier access
	LPC_TMR_TypeDef* timer[TIMER_COUNT];
#elif defined(LPC1785)
#warning "implementation for LPC1785 is missing"
#else
#error "No controller defined"
#endif

	/// @brief: Callback function for all timers
	void (*timer_callbacks[TIMER_COUNT])(void* user_ptr,
			uw_timer_int_sources_e source,
			unsigned int timer_value);
	/// @brief: Callback function for tick timer
	void (*tick_callback)(void* user_ptr, uint32_t step_ms);
	uint32_t tick_timer_cycle_time_ms;
} this_st;

this_st _this;
#define this (&_this)


// timer interrupt handler
static void parse_timer_interrupt(uw_timers_e timer) {
#ifdef LPC11C14
	// check trough all interrupts and call interrupt handler for all pending interrupts
	// match 3 interrupt == overflow interrupt
	if (this->timer[timer]->IR & (1 << 3)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(),
				INT_SRC_OVERFLOW, this->timer[timer]->MR3);
	}
	if (this->timer[timer]->IR & (1 << 4)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(),
				INT_SRC_CAPTURE, this->timer[timer]->CR0);
	}
	// clear all timer interrupts
	this->timer[timer]->IR = 0x7F;

	#elif defined(LPC1785)
#warning "parse_timer_interrupt for lpc1785 is missing"
#endif
}

/// @brief: Initializes the timer struct
static void this_init() {
#ifdef LPC11C14
	this->timer[CT16B0] = LPC_TMR16B0;
	this->timer[CT16B1] = LPC_TMR16B1;
	this->timer[CT32B0] = LPC_TMR32B0;
	this->timer[CT32B1] = LPC_TMR32B1;
#elif defined(LPC1785)
#warning "this_init for lpc1785 is missing"
#endif
	int i;
	for (i = 0; i< TIMER_COUNT; i++) {
		this->timer_callbacks[i] = NULL;
	}
	this->tick_timer_cycle_time_ms = 0;
	this->tick_callback = NULL;

}

// checks if this hardware has a timer of this value
static bool validate_timer(uw_timers_e timer) {
	if (timer >= TIMER_COUNT) {
		printf("Warning: Timer %u not found in this hardware\n\r", timer);
		return false;
	}
	return true;
}

// interrupt handlers
#ifdef LPC11C14
void TIMER16_0_IRQHandler(void) {
	parse_timer_interrupt(CT16B0);
}
void TIMER16_1_IRQHandler(void) {
	parse_timer_interrupt(CT16B1);
}
void TIMER32_0_IRQHandler(void) {
	parse_timer_interrupt(CT32B0);
}
void TIMER32_1_IRQHandler(void) {
	parse_timer_interrupt(CT32B1);
}
#elif defined(LPC1785)

#warning "Timer interrupt routines not defined for lpc1785"

#else
#error "Controller not defined!"
#endif



bool uw_timer_init(uw_timers_e timer, uw_timer_init_modes_e init_mode,
		unsigned int init_value, unsigned int fosc) {
	this_init();
	if (!validate_timer(timer)) return false;

	//set prescaler depending on in which mode init_value was given
	unsigned long int prescaler;
	unsigned int max_value = 0xFFFF;
	if (timer > CT16B1) {
		max_value = 0xFFFFFFFF;
	}
	switch (init_mode) {
	case TIMER_PRESCALER:
		prescaler = init_value;
		break;
	case TIMER_OVERFLOW_FREQ_HZ:
	case TIMER_OVERFLOW_TIME_US:
		prescaler = (unsigned long int) fosc /
					((unsigned long int) init_value * max_value) - 1;
		// now we have prescaler when defined as a frequency
		if (init_mode == TIMER_OVERFLOW_FREQ_HZ) {
			break;
		}
		// time step is an inverse of the value
		prescaler = 1000000 / prescaler;
		break;
	case TIMER_STEP_FREQ_HZ:
	case TIMER_STEP_TIME_US:
		prescaler = (unsigned long int) fosc * (max_value + 1) /
		((unsigned long int) init_value * max_value) - 1;
		// now wh have precaler when defined a a frequency
		if (init_mode == TIMER_STEP_FREQ_HZ) {
			break;
		}
		// time step is an inverse of the value
		prescaler = 1000000 / prescaler;
		break;
	default:
		printf("Warning: Unknown timer init mode %u\n\r", init_mode);
		return false;
	}
	if (prescaler > max_value) {
		printf("Warning: Invalid init value in timer initialization. Check that the frequency or\
 step time required is in a valid range.\n\r");
	}

#ifdef LPC11C14
	//enable sys clock to the timer module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << (timer + 7));

	// add prescaler
	this->timer[timer]->PR = prescaler;

	// start timer
	this->timer[timer]->TCR |= 1 << 0;

	#elif defined(LPC1785)
#warning "uw_timer_init for LPC1785 is missing"
	return false;
#endif

	return true;
}



bool uw_pwm_init(uw_timers_e timer, uw_timer_pwms_e channels,
		unsigned int freq, unsigned int fosc) {
	if (!validate_timer(timer)) return false;
	this_init();
#ifdef LPC11C14

	unsigned long int prescaler;
	unsigned int max_value = 0xFFFF;
	if (timer > CT16B1) {
		max_value = 0xFFFFFFFF;
	}
	prescaler = (unsigned long int) fosc /
				((unsigned long int) freq * max_value) - 1;
	if (prescaler > max_value) {
		printf("Warning: Invalid init value in timer pwm initialization. Check that the \
frequency required is in a valid range.\n\r");
		return false;
	}

	//enable sys clock to the timer module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << (timer + 7));

	// add prescaler
	this->timer[timer]->PR = prescaler;

	// enable PWM outputs
	this->timer[timer]->PWMC = channels;

	// clear tmr clear and stop for match 0, 1 and 2
	this->timer[timer]->MCR &= ~0x3FF;

	// generate interrupt and reset timer on match 3
	this->timer[timer]->MCR |= (0b1 << 10);

	// set match 3 to timer max value
	// (16-bit timer values will be clamped to 16-bit value)
	this->timer[timer]->MR3 = 0xFFFFFFFF;

	// start timer
	this->timer[timer]->TCR |= 1 << 0;


#elif defined(LPC1785)
#warning "uw_pwm_init for lpc1785 is missing"
	return false;
#endif

	return true;
}


bool uw_counter_init(uw_timers_e timer, uw_timer_captures_e capture_input,
		uw_timer_capture_edges_e capture_edge) {
	if (!validate_timer(timer)) return false;
	this_init();
#ifdef LPC11C14
	printf("Error: UW Counter mode not implemented yet in HAL library\n\r");
	return false;
#elif defined(LPC1785)
#warning "uw_counter_init for lpc1785 is missing"
	return false;
#endif

	return true;
}


int uw_timer_get_value(uw_timers_e timer) {
	validate_timer(timer);
#ifdef LPC11C14
	return this->timer[timer]->TC;
#elif defined(LPC1785)
#warning "uw_timer_get_value for lpc1785 is missing"
#endif
}



bool uw_timer_clear(uw_timers_e timer) {
	if (!validate_timer(timer)) return false;
	if (timer == TICK_TIMER) {
		printf("Warning: Can't clear tick timer\n\r");
		return false;
	}
#ifdef LPC11C14
	this->timer[timer]->TC = 0;
#elif defined(LPC1785)
#warning "uw_timer_clear for lpc1785 is missing"
#endif
	return true;
}


bool uw_timer_add_capture(uw_timers_e timer, uw_timer_captures_e input) {
	if (!validate_timer(timer)) return false;
#ifdef LPC11C14
	// clear capture control register to default value
	this->timer[timer]->CCR = 0;

	if (input < CAPTURE_0_BOTH_EDGES) {
		this->timer[timer]->CCR |= (1 << input);
	}
	else if (input < CAPTURE_COUNT) {
		this->timer[timer]->CCR |= 0b11;
	}
	else {
		printf("Warning: Unknown capture input %u\n\r", input);
		return false;
	}
	// set enable capture interrupt bit
	this->timer[timer]->CCR |= (1 << 2);
	switch (timer) {
	case CT16B0:
		//set TMR16B0CAP0 pin to capture mode
		LPC_IOCON->PIO0_2 &= ~0b111;
		LPC_IOCON->PIO0_2 |= 0x2;
		break;
	case CT16B1:
		LPC_IOCON->PIO1_8 &= ~0b111;
		LPC_IOCON->PIO1_8 |= 0x1;
		break;
	case CT32B0:
		LPC_IOCON->PIO1_5 &= ~0b111;
		LPC_IOCON->PIO1_5 |= 0x2;
		break;
	case CT32B1:
		LPC_IOCON->R_PIO1_0 &= ~0b111;
		LPC_IOCON->R_PIO1_0 |= 0x3;
		break;
	default:
		break;
	}
	#elif defined(LPC1785)
#warning "uw_timer_add_capture for lpc1785 is missing"
	return false;
#endif

	return true;
}


bool uw_pwm_set(uw_timers_e timer, uw_timer_pwms_e channel, unsigned int duty_cycle) {
	if (!validate_timer(timer)) return false;
#ifdef LPC11C14

	unsigned int max_value = this->timer[timer]->MR3;
	// clamp duty cycle to max value
	if (duty_cycle > 1000) {
		duty_cycle = 1000;
	}
	// calculate register value from duty cycle
	unsigned int value = max_value / 1000 * (1000 - duty_cycle);
	switch(channel) {
	case PWM_CHANNEL_0:
		this->timer[timer]->MR0 = value;
		break;
	case PWM_CHANNEL_1:
		this->timer[timer]->MR1 = value;
		break;
	case PWM_CHANNEL_2:
		this->timer[timer]->MR2 = value;
		break;
	default:
		printf("Warning: Unknown PWM channel %u\n\r", channel);
		break;
	}

	#elif defined(LPC1785)
#warning "uw_pwm_set for lpc1785 is missing"
	return false;
#endif

	return true;
}


bool uw_timer_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timer_int_sources_e source, unsigned int value)) {
	if (!validate_timer(timer)) return false;

	// enable interrupts on this timer
	IRQn_Type tmr;
	switch (timer) {
	case CT16B0: tmr = TIMER_16_0_IRQn; break;
	case CT16B1: tmr = TIMER_16_1_IRQn; break;
	case CT32B0: tmr = TIMER_32_0_IRQn; break;
	case CT32B1: tmr = TIMER_32_1_IRQn; break;
	default:
		return false;
	}
	NVIC_EnableIRQ(tmr);

	// generate interrupt and reset timer on match 3
	this->timer[timer]->MCR |= (0b11 << 9);

	// set match 3 to timer max value
	// (16-bit timer values will be clamped to 16-bit value)
	this->timer[timer]->MR3 = 0xFFFFFFFF;

	// remember callback function pointer
	this->timer_callbacks[timer] = callback_function;

	return true;
}







// tick timer interrupt handler
inline void SysTick_Handler(void) {
	if (this->tick_callback != 0) {
		this->tick_callback(__uw_get_user_ptr(), this->tick_timer_cycle_time_ms);
	}
}



bool uw_tick_timer_init(uint32_t freq, uint32_t fosc) {
	//enable timer interrupts
	NVIC_EnableIRQ(SysTick_IRQn);
	this->tick_timer_cycle_time_ms = 1000 / freq;
	if (SysTick_Config(fosc / freq)) {
		//error happened
		printf("Tick timer encountered an error. \
Tried to init with too low or too high frequency?\n\r");
		return false;
	}
	return true;

}

void uw_tick_timer_add_callback(
		void (*callback_function_ptr)(void*, uint32_t)) {
	this->tick_callback = callback_function_ptr;
}
