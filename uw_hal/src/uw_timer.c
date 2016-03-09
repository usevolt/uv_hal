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
#include "LPC177x_8x.h"
#endif

typedef struct {
#ifdef LPC11C14
	/// @brief: All timer pointers on this hardware for easier access
	LPC_TMR_TypeDef* timer[TIMER_COUNT];
#elif defined(LPC1785)
	LPC_TIM_TypeDef* timer[TIMER_COUNT];
	LPC_PWM_TypeDef* pwm[PWM_COUNT];
#else
#error "No controller defined"
#endif

	/// @brief: Callback function for all timers
	void (*timer_callbacks[TIMER_COUNT])(void* user_ptr,
			uw_timers_e timer,
			uw_timer_int_sources_e source,
			unsigned int timer_value);
	/// @brief: Callback function for tick timer
	void (*tick_callback)(void* user_ptr, uint32_t step_ms);
	uint32_t tick_timer_cycle_time_ms;
} this_st;

this_st _this;
#define this (&_this)


// checks if this hardware has a timer of this value
static uw_errors_e validate_timer(uw_timers_e timer) {
	if (timer >= TIMER_COUNT) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}
	return ERR_NONE;
}
#if defined(LPC1785)
// checks if this hardware has a pwm of this value
static uw_errors_e validate_pwm(uw_pwms_e pwm) {
	if (pwm >= PWM_COUNT) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}
	return ERR_NONE;
}
#endif



/// @brief: Initializes the timer struct
static void this_init(uw_timers_e timer) {
#ifdef LPC11C14
	this->timer[TIMER0] = LPC_TMR16B0;
	this->timer[TIMER1] = LPC_TMR16B1;
	this->timer[TIMER2] = LPC_TMR32B0;
	this->timer[TIMER3] = LPC_TMR32B1;
#elif defined(LPC1785)
	this->timer[TIMER0] = LPC_TIM0;
	this->timer[TIMER1] = LPC_TIM1;
	this->timer[TIMER2] = LPC_TIM2;
	this->timer[TIMER3] = LPC_TIM3;
	this->pwm[PWM0] = LPC_PWM0;
	this->pwm[PWM1] = LPC_PWM1;
#endif
	if (validate_timer(timer)) return;
	this->timer_callbacks[timer] = NULL;
}


// timer interrupt handler
static void parse_timer_interrupt(uw_timers_e timer) {
#ifdef LPC11C14
	// check trough all interrupts and call interrupt handler for all pending interrupts
	// match 3 interrupt == overflow interrupt
	if (this->timer[timer]->IR & (1 << 3)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_OVERFLOW, this->timer[timer]->MR3);
	}
	if (this->timer[timer]->IR & (1 << 4)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_CAPTURE0, this->timer[timer]->CR0);
	}
	// clear all timer interrupts
	this->timer[timer]->IR = 0x1F;

#elif defined(LPC1785)
	// check trough all interrupts and call interrupt handler for all pending interrupts
	// match 3 interrupt == overflow interrupt
	if (this->timer[timer]->IR & (1 << 3)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_OVERFLOW, this->timer[timer]->MR3);
	}
	if (this->timer[timer]->IR & (1 << 4)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_CAPTURE0, this->timer[timer]->CR0);
	}
	if (this->timer[timer]->IR & (1 << 5)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_CAPTURE1, this->timer[timer]->CR1);
	}
	// clear all timer interrupts
	this->timer[timer]->IR = 0x3F;
#endif
}

// interrupt handlers
#ifdef LPC11C14
void TIMER16_0_IRQHandler(void) {
	parse_timer_interrupt(TIMER0);
}
void TIMER16_1_IRQHandler(void) {
	parse_timer_interrupt(TIMER1);
}
void TIMER32_0_IRQHandler(void) {
	parse_timer_interrupt(TIMER2);
}
void TIMER32_1_IRQHandler(void) {
	parse_timer_interrupt(TIMER3);
}
#elif defined(LPC1785)
void TIMER0_IRQHandler(void) {
	parse_timer_interrupt(TIMER0);
}
void TIMER1_IRQHandler(void) {
	parse_timer_interrupt(TIMER1);
}
void TIMER2_IRQHandler(void) {
	parse_timer_interrupt(TIMER2);
}
void TIMER3_IRQHandler(void) {
	parse_timer_interrupt(TIMER3);
}
#endif



uw_errors_e uw_timer_init(uw_timers_e timer, float freq) {
	if (validate_timer(timer)) return validate_timer(timer);
	this_init(timer);
	SystemCoreClockUpdate();

#ifdef LPC11C14
	//set prescaler depending on in which mode init_value was given
	uint64_t prescaler;
	uint32_t max_value = 0xFFFF;
	if (timer > TIMER1) {
		max_value = 0xFFFFFFFF;
	}
	prescaler = (float) SystemCoreClock /
				((float) freq * max_value) - 1;
	// now we have prescaler when defined as a frequency
	// if the freq is too high for timer to calculate to it's max value,
	// decrease the max value accordingly
	if (prescaler == 0) {
		max_value = SystemCoreClock / freq;
	}

	//enable sys clock to the timer module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << (timer + 7));
	// add prescaler
	this->timer[timer]->PR = prescaler;
	// reset timer on match 3
	this->timer[timer]->MCR = (0b1 << 10);
	// set match 3 to timer max value
	this->timer[timer]->MR3 = max_value;
	// reset timer value
	uw_timer_stop(timer);
	uw_timer_clear(timer);

	#elif defined(LPC1785)
	//set prescaler depending on in which mode init_value was given
	uint64_t prescaler;
	uint32_t max_value = 0xFFFFFFFF;
	prescaler = (float) PeripheralClock /
				((float) freq * max_value) - 1;
	// if the freq is too high for timer to calculate to it's max value,
	// decrease the max value accordingly
	if (prescaler == 0) {
		max_value = PeripheralClock / freq;
	}

	//enable sys clock to the timer module
	if (timer <= TIMER1) {
		LPC_SC->PCONP |= (1 << (timer + 1));
	}
	else {
		LPC_SC->PCONP |= (1 << (timer + 20));
	}
	// add prescaler
	this->timer[timer]->PR = prescaler;
	// reset timer on match 3
	this->timer[timer]->MCR = (1 << 10);
	// set match 3 to timer max value
	this->timer[timer]->MR3 = max_value;
	// reset timer value
	uw_timer_stop(timer);
	uw_timer_clear(timer);
#endif

	return ERR_NONE;
}





int uw_timer_get_value(uw_timers_e timer) {
	if (validate_timer(timer)) return -1;
	return this->timer[timer]->TC;
}

uw_errors_e uw_timer_start(uw_timers_e timer) {
	if (validate_timer(timer)) return validate_timer(timer);
	// start timer
	this->timer[timer]->TCR |= 1 << 0;
	return ERR_NONE;
}

uw_errors_e uw_timer_stop(uw_timers_e timer) {
	if (validate_timer(timer)) return validate_timer(timer);
	// stop timer
	this->timer[timer]->TCR &= ~(1);
	return ERR_NONE;
}


uw_errors_e uw_timer_clear(uw_timers_e timer) {
	if (validate_timer(timer)) return validate_timer(timer);
	this->timer[timer]->TCR |= (1 << 1);
	__NOP();
	this->timer[timer]->TCR &= ~(1 << 1);
	return ERR_NONE;
}

uw_errors_e uw_timer_set_freq(uw_timers_e timer, float freq) {
	if (validate_timer(timer)) return validate_timer(timer);

	 SystemCoreClockUpdate();

	// stop and clear timer
	uw_timer_stop(timer);
	uw_timer_clear(timer);

	//set prescaler depending on in which mode init_value was given
	uint64_t prescaler;
#ifdef LPC11C14
	uint32_t max_value = 0xFFFF;
	if (timer > TIMER1) {
		max_value = 0xFFFFFFFF;
	}
	prescaler = (float) SystemCoreClock /
				((float) freq * max_value) - 1;
	// now we have prescaler when defined as a frequency
	// if the freq is too high for timer to calculate to it's max value,
	// decrease the max value accordingly
	if (prescaler == 0) {
		max_value = SystemCoreClock / freq;
	}
#elif defined(LPC1785)
	uint32_t max_value = 0xFFFFFFFF;
	prescaler = (float) PeripheralClock /
				((float) freq * max_value) - 1;
	if (prescaler == 0) {
		max_value = PeripheralClock / freq;
	}
#endif

	// add prescaler
	this->timer[timer]->PR = prescaler;

	// set match 3 to timer max value
	this->timer[timer]->MR3 = max_value;

	return ERR_NONE;
}



#ifdef LPC11C14
uw_errors_e uw_timer_add_capture(uw_timers_e timer, uw_timer_captures_e capture,
		uw_timer_capture_modes_e input) {
	if (validate_timer(timer)) return validate_timer(timer);
	if (capture >= CAPTURE_COUNT) {
		__uw_err_throw(ERR_UNSUPPORTED_PARAM2_VALUE | HAL_MODULE_TIMER);
	}
	// clear capture control register to default value
	this->timer[timer]->CCR = 0;

	if (input < CAPTURE_BOTH_EDGES) {
		this->timer[timer]->CCR |= (1 << input);
	}
	else if (input < CAPTURE_MODE_COUNT) {
		this->timer[timer]->CCR |= 0b11;
	}
	else {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}
	// set enable capture interrupt bit
	this->timer[timer]->CCR |= (1 << 2);
	switch (timer) {
	case TIMER0:
		//set TMR16B0CAP0 pin to capture mode
		LPC_IOCON->PIO0_2 &= ~0b111;
		LPC_IOCON->PIO0_2 |= 0x2;
		break;
	case TIMER1:
		LPC_IOCON->PIO1_8 &= ~0b111;
		LPC_IOCON->PIO1_8 |= 0x1;
		break;
	case TIMER2:
		LPC_IOCON->PIO1_5 &= ~0b111;
		LPC_IOCON->PIO1_5 |= 0x2;
		break;
	case TIMER3:
		LPC_IOCON->R_PIO1_0 &= ~0b111;
		LPC_IOCON->R_PIO1_0 |= 0x3;
		break;
	default:
		__uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_TIMER);
	}
	return ERR_NONE;
}

#elif defined(LPC1785)
uw_errors_e uw_timer_add_capture(uw_timers_e timer, uw_timer_captures_e capture,
		uw_timer_capture_modes_e input, uw_capture_pins_e capture_pin) {

	if (validate_timer(timer)) return validate_timer(timer);
	if (capture >= CAPTURE_COUNT) {
		__uw_err_throw(ERR_UNSUPPORTED_PARAM2_VALUE | HAL_MODULE_TIMER);
	}

	if (input < CAPTURE_BOTH_EDGES) {
		this->timer[timer]->CCR |= (1 << (input + capture * 3));
	}
	else if (input < CAPTURE_MODE_COUNT) {
		this->timer[timer]->CCR |= (0b11 << (capture * 3));
	}
	else {
		__uw_err_throw(ERR_UNSUPPORTED_PARAM4_VALUE | HAL_MODULE_TIMER);
	}
	// set enable capture interrupt bit
	this->timer[timer]->CCR |= (1 << (2 + capture * 3));
	(*__uw_gpio_get_iocon(capture_pin)) &= ~0b111;
	(*__uw_gpio_get_iocon(capture_pin)) |= 0b011;

	return ERR_NONE;
}
#endif



#ifdef LPC11C14
uw_errors_e uw_pwm_init(uw_timers_e timer, uw_pwm_channels_e channels,
		unsigned int freq) {
	if (validate_timer(timer)) return validate_timer(timer);
	this_init(timer);

	SystemCoreClockUpdate();

	unsigned long int prescaler;
	unsigned int max_value = 1000;
	prescaler = (unsigned long int) SystemCoreClock /
				((unsigned long int) freq * max_value) - 1;
	if (prescaler > max_value) {
		__uw_err_throw(ERR_FREQ_TOO_HIGH_OR_TOO_LOW | HAL_MODULE_TIMER);
	}

	//enable sys clock to the timer module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << (timer + 7));

	// add prescaler
	this->timer[timer]->PR = prescaler;

	// enable PWM outputs
	this->timer[timer]->PWMC = channels;

	// clear tmr clear and stop for match 0, 1 and 2
	this->timer[timer]->MCR &= ~0x3FF;

	// reset timer on match 3
	this->timer[timer]->MCR |= (0b1 << 10);

	// set match 3 to timer max value
	this->timer[timer]->MR3 = max_value;

	// start timer
	this->timer[timer]->TCR |= 1 << 0;



	return ERR_NONE;
}
#elif defined(LPC1785)
uw_errors_e uw_pwm_init(uw_pwms_e pwm, uw_pwm_channels_e channels,
		unsigned int freq) {
	if (validate_pwm(pwm)) return validate_pwm(pwm);

	SystemCoreClockUpdate();

	#warning "uw_pwm_init for lpc1785 is missing"
	__uw_err_throw(ERR_NOT_IMPLEMENTED | HAL_MODULE_TIMER);
	return ERR_NONE;
}
#endif




#ifdef LPC11C14
uw_errors_e uw_pwm_set(uw_timers_e timer, uw_pwm_channels_e channel, unsigned int duty_cycle) {
	if (validate_timer(timer)) return validate_timer(timer);

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
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}


	return ERR_NONE;
}
#elif defined(LPC1785)
uw_errors_e uw_pwm_set(uw_pwms_e pwm, uw_pwm_channels_e channel, unsigned int duty_cycle) {
#warning "uw_pwm_set for lpc1785 is missing"
	__uw_err_throw(ERR_NOT_IMPLEMENTED | HAL_MODULE_TIMER);
	return ERR_NONE;

}
#endif




uw_errors_e uw_counter_init(uw_timers_e timer, uw_timer_capture_modes_e capture_input,
		uw_timer_capture_edges_e capture_edge) {
	if (validate_timer(timer)) return validate_timer(timer);
	this_init(timer);
#ifdef LPC11C14
	__uw_err_throw(ERR_NOT_IMPLEMENTED | HAL_MODULE_TIMER);
#elif defined(LPC1785)
#warning "uw_counter_init for lpc1785 is missing"
	__uw_err_throw(ERR_NOT_IMPLEMENTED | HAL_MODULE_TIMER);
#endif

	return ERR_NONE;
}





uw_errors_e uw_timer_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timers_e timer, uw_timer_int_sources_e source, unsigned int value)) {
	if (validate_timer(timer)) return validate_timer(timer);

	// enable interrupts on this timer
	IRQn_Type tmr;
	switch (timer) {
#ifdef LPC11C14
	case TIMER0: tmr = TIMER_16_0_IRQn; break;
	case TIMER1: tmr = TIMER_16_1_IRQn; break;
	case TIMER2: tmr = TIMER_32_0_IRQn; break;
	case TIMER3: tmr = TIMER_32_1_IRQn; break;
#elif defined(LPC1785)
	case TIMER0: tmr = TIMER0_IRQn; break;
	case TIMER1: tmr = TIMER1_IRQn; break;
	case TIMER2: tmr = TIMER2_IRQn; break;
	case TIMER3: tmr = TIMER3_IRQn; break;
#endif
	default:
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}
	NVIC_EnableIRQ(tmr);

	// generate interrupt on match 3
	this->timer[timer]->MCR |= (0b11 << 9);

	// remember callback function pointer
	this->timer_callbacks[timer] = callback_function;

	return ERR_NONE;
}




#if !USE_FREERTOS

// tick timer interrupt handler
inline void SysTick_Handler(void) {
	if (this->tick_callback != 0) {
		this->tick_callback(__uw_get_user_ptr(), this->tick_timer_cycle_time_ms);
	}
}



uw_errors_e uw_tick_timer_init(uint32_t freq) {
	SystemCoreClockUpdate();


	//enable timer interrupts
	this->tick_timer_cycle_time_ms = 1000 / freq;
	if (SysTick_Config(SystemCoreClock / freq)) {
		//error happened
		__uw_err_throw(ERR_FREQ_TOO_HIGH_OR_TOO_LOW | HAL_MODULE_TIMER);
	}
	return ERR_NONE;

}

void uw_tick_timer_add_callback(
		void (*callback_function_ptr)(void*, uint32_t)) {
	this->tick_callback = callback_function_ptr;
}

#endif
