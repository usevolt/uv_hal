/*
 * uw_timer_controller.c
 *
 *  Created on: Jan 28, 2015
 *      Author: usenius
 */


#include "uw_timer.h"


#include <stdlib.h>
#include <stdio.h>
#include "uw_utilities.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif

typedef struct {
#if CONFIG_TARGET_LPC11C14
	/// @brief: All timer pointers on this hardware for easier access
	LPC_TMR_TypeDef* timer[TIMER_COUNT];
#elif CONFIG_TARGET_LPC1785
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

this_st _this = {
		.timer_callbacks = { NULL, NULL, NULL, NULL }
};
#define this (&_this)


// checks if this hardware has a timer of this value
static uw_errors_e validate_timer(uw_timers_e timer) {
	if (timer >= TIMER_COUNT) {
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}
	return uw_err(ERR_NONE);
}



/// @brief: Initializes the timer struct
static void this_init(uw_timers_e timer) {
#if CONFIG_TARGET_LPC11C14
	this->timer[0] = LPC_TMR16B0;
	this->timer[1] = LPC_TMR16B1;
	this->timer[2] = LPC_TMR32B0;
	this->timer[3] = LPC_TMR32B1;
#elif CONFIG_TARGET_LPC1785
	this->timer[0] = LPC_TIM0;
	this->timer[1] = LPC_TIM1;
	this->timer[2] = LPC_TIM2;
	this->timer[3] = LPC_TIM3;
	this->pwm[0] = LPC_PWM0;
	this->pwm[1] = LPC_PWM1;
#endif
}


// timer interrupt handler

static void parse_timer_interrupt(uw_timers_e timer) {

	// check trough all interrupts and call interrupt handler for all pending interrupts
	// match 3 interrupt == overflow interrupt
	if (!this->timer_callbacks[timer]) {
		this->timer[timer]->IR = 0x1F;
		return;
	}
	if (this->timer[timer]->IR & (1 << 3)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_OVERFLOW, this->timer[timer]->MR3);
	}
	if (this->timer[timer]->IR & (1 << 4)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_CAPTURE0, this->timer[timer]->CR0);
	}
#if CONFIG_TARGET_LPC11C14
	// clear all timer interrupts
	this->timer[timer]->IR = 0x1F;
#elif CONFIG_TARGET_LPC1785
	if (this->timer[timer]->IR & (1 << 5)) {
		this->timer_callbacks[timer](__uw_get_user_ptr(), timer,
				INT_SRC_CAPTURE1, this->timer[timer]->CR1);
	}
	// clear all timer interrupts
	this->timer[timer]->IR = 0x3F;
#endif
}

// interrupt handlers
#if CONFIG_TARGET_LPC11C14
#if CONFIG_TIMER0
void TIMER16_0_IRQHandler(void) {
	parse_timer_interrupt(TIMER0);
}
#endif
#if CONFIG_TIMER1
void TIMER16_1_IRQHandler(void) {
	parse_timer_interrupt(TIMER1);
}
#endif
#if CONFIG_TIMER2
void TIMER32_0_IRQHandler(void) {
	parse_timer_interrupt(TIMER2);
}
#endif
#if CONFIG_TIMER3
void TIMER32_1_IRQHandler(void) {
	parse_timer_interrupt(TIMER3);
}
#endif
#elif CONFIG_TARGET_LPC1785
#if CONFIG_TIMER0
void TIMER0_IRQHandler(void) {
	parse_timer_interrupt(TIMER0);
}
#endif
#if CONFIG_TIMER1
void TIMER1_IRQHandler(void) {
	parse_timer_interrupt(TIMER1);
}
#endif
#if CONFIG_TIMER2
void TIMER2_IRQHandler(void) {
	parse_timer_interrupt(TIMER2);
}
#endif
#if CONFIG_TIMER3
void TIMER3_IRQHandler(void) {
	parse_timer_interrupt(TIMER3);
}
#endif
#endif


#if (CONFIG_TIMER0 || CONFIG_TIMER1 || CONFIG_TIMER2 || CONFIG_TIMER3)
static void init_timer(unsigned int timer, unsigned int freq) {
#if CONFIG_TARGET_LPC11C14
	// enable clock to the timer module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << (7 + timer));

	// set frequency
	uw_timer_set_freq(timer, freq);

	// reset timer value
	uw_timer_stop(timer);
	uw_timer_clear(timer);

	// generate interrupt on match 3
	this->timer[timer]->MCR |= (0b11 << 9);

	// set enable capture interrupt bit
	this->timer[timer]->CCR |= (1 << 2);

	switch (timer) {
#if CONFIG_TIMER0
	case TIMER0:
#if CONFIG_TIMER0_CAP0_FALLING_EDGES
		LPC_TMR16B0->CCR |= (0b110);
#endif
#if CONFIG_TIMER0_CAP0_RISING_EDGES
		LPC_TMR16B0->CCR |= 0b101;
#endif
#if CONFIG_TIMER0_CAP0_PIO0_2
		LPC_IOCON->PIO0_2 &= ~0b111;
		LPC_IOCON->PIO0_2 |= 0x2;
#endif
		break;
#endif
#if CONFIG_TIMER1
	case TIMER1:
#if CONFIG_TIMER1_CAP0_FALLING_EDGES
		LPC_TMR16B1->CCR |= 0b110;
#endif
#if CONFIG_TIMER1_CAP0_RISING_EDGES
		LPC_TMR16B1->CCR |= 0b101;
#endif
#if CONFIG_TIMER1_CAP0_PIO1_8
		LPC_IOCON->PIO1_8 &= ~0b111;
		LPC_IOCON->PIO1_8 |= 0x1;
#endif
		break;
#endif
#if CONFIG_TIMER2
	case TIMER2:
#if CONFIG_TIMER2_CAP0_FALLING_EDGES
		LPC_TMR32B0->CCR |= 0b110;
#endif
#if CONFIG_TIMER2_CAP0_RISING_EDGES
		LPC_TMR32B0->CCR |= 0b101;
#endif
#if CONFIG_TIMER2_CAP0_PIO1_5
		LPC_IOCON->PIO1_5 &= ~0b111;
		LPC_IOCON->PIO1_5 |= 0x2;
#endif
		break;
#endif
#if CONFIG_TIMER3
	case TIMER3:
#if CONFIG_TIMER3_CAP0_FALLING_EDGES
		LPC_TMR32B1->CCR |= 0b110;
#endif
#if CONFIG_TIMER3_CAP0_RISING_EDGES
		LPC_TMR32B1->CCR |= 0b101;
#endif
#if CONFIG_TIMER3_CAP0_PIO1_0
		LPC_IOCON->PIO1_0 &= ~0b111;
		LPC_IOCON->PIO1_0 |= 0x3;
#endif
		break;
#endif
	default:
		break;
	}


#elif CONFIG_TARGET_LPC1785
	// set frequency
	uw_timer_set_freq(timer, freq);

	//enable sys clock to the timer module
	if (timer <= TIMER1) {
		LPC_SC->PCONP |= (1 << (timer + 1));
	}
	else {
		LPC_SC->PCONP |= (1 << (timer + 20));
	}
	// reset timer on match 3
	this->timer[timer]->MCR = (1 << 10);
	// reset timer value
	uw_timer_stop(timer);
	uw_timer_clear(timer);

	// generate interrupt on match 3
	this->timer[timer]->MCR |= (0b11 << 9);

	unsigned int ccr = 0;
	volatile uint32_t *iocon_cap0 = NULL;
	volatile uint32_t *iocon_cap1 = NULL;
	// Capture inputs
	switch (timer) {
#if CONFIG_TIMER0
	case TIMER0:
#if CONFIG_TIMER0_CAP0_FALLING_EDGES
		ccr |= 0b110;
#endif
#if CONFIG_TIMER0_CAP0_RISING_EDGES
		ccr |= (0b101);
#endif
#if CONFIG_TIMER0_CAP1_FALLING_EDGES
		ccr |= (0b110 << 3);
#endif
#if CONFIG_TIMER0_CAP1_RISING_EDGES
		ccr|= (0b101 << 3);
#endif
#if CONFIG_TIMER0_CAP0_PIO3_23
		iocon_cap0 = &LPC_IOCON->P3_23;
#endif
#if CONFIG_TIMER0_CAP1_PIO3_24
		iocon_cap1 = &LPC_IOCON->P3_24;
#endif
		break;
#endif
#if CONFIG_TIMER1
	case TIMER1:
#if CONFIG_TIMER1_CAP0_FALLING_EDGES
		ccr |= 0b110;
#endif
#if CONFIG_TIMER1_CAP0_RISING_EDGES
		ccr |= (0b101);
#endif
#if CONFIG_TIMER1_CAP1_FALLING_EDGES
		ccr |= (0b110 << 3);
#endif
#if CONFIG_TIMER1_CAP1_RISING_EDGES
		ccr|= (0b101 << 3);
#endif
#if CONFIG_TIMER1_CAP0_PIO3_27
		iocon_cap0 = &LPC_IOCON->P3_27;
#endif
#if CONFIG_TIMER1_CAP1_PIO1_19
		iocon_cap1 = &LPC_IOCON->P1_19;
#endif
#if CONFIG_TIMER1_CAP1_PIO3_28
		iocon_cap1 = &LPC_IOCON->P3_28;
#endif
		break;
#endif
#if CONFIG_TIMER2
	case TIMER2:
#if CONFIG_TIMER2_CAP0_FALLING_EDGES
		ccr |= 0b110;
#endif
#if CONFIG_TIMER2_CAP0_RISING_EDGES
		ccr |= (0b101);
#endif
#if CONFIG_TIMER2_CAP1_FALLING_EDGES
		ccr |= (0b110 << 3);
#endif
#if CONFIG_TIMER2_CAP1_RISING_EDGES
		ccr|= (0b101 << 3);
#endif
#if CONFIG_TIMER2_CAP0_PIO0_4
		iocon_cap0 = &LPC_IOCON->P0_4;
#endif
#if CONFIG_TIMER2_CAP0_PIO1_14
		iocon_cap0 = &LPC_IOCON->P1_14;
#endif
#if CONFIG_TIMER2_CAP0_PIO2_14
		iocon_cap0 = &LPC_IOCON->P2_14;
#endif
#if CONFIG_TIMER2_CAP0_PIO2_6
		iocon_cap0 = &LPC_IOCON->P2_6;
#endif
#if CONFIG_TIMER2_CAP1_PIO0_5
		iocon_cap1 = &LPC_IOCON->P0_5;
#endif
#if CONFIG_TIMER2_CAP1_PIO2_15
		iocon_cap1 = &LPC_IOCON->P2_15;
#endif
		break;
#endif
#if CONFIG_TIMER3
	case TIMER3:
#if CONFIG_TIMER3_CAP0_FALLING_EDGES
		ccr |= 0b110;
#endif
#if CONFIG_TIMER3_CAP0_RISING_EDGES
		ccr |= (0b101);
#endif
#if CONFIG_TIMER3_CAP1_FALLING_EDGES
		ccr |= (0b110 << 3);
#endif
#if CONFIG_TIMER3_CAP1_RISING_EDGES
		ccr|= (0b101 << 3);
#endif
#if CONFIG_TIMER3_CAP0_PIO1_10
		iocon_cap0 = &LPC_IOCON->P1_10;
#endif
#if CONFIG_TIMER3_CAP0_PIO2_22
		iocon_cap0 = &LPC_IOCON->P2_22;
#endif
#if CONFIG_TIMER3_CAP1_PIO2_23
		iocon_cap1 = &LPC_IOCON->P2_23;
#endif
		break;
#endif
	default:
		break;
	}
	this->timer[timer]->CCR = ccr;
	// set capture input iocons
	if (iocon_cap0) {
		(*iocon_cap0) &= ~(0b111);
		(*iocon_cap0) |= 0b011;
	}
	if (iocon_cap1) {
		(*iocon_cap1) &= ~(0b111);
		(*iocon_cap1) |= 0b111;
	}


#endif
}
#endif

#if (CONFIG_PWM0 || CONFIG_PWM1 || CONFIG_PWM2 || CONFIG_PWM3)
static void init_pwm(unsigned int timer, unsigned int freq) {

#if CONFIG_TARGET_LPC11C14
	// enable clock to the timer module
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << (7 + timer));

	// set PWM frequency
	uw_timer_set_freq(timer, freq);

	switch (timer) {
#if CONFIG_PWM0
	case PWM0:
#if CONFIG_PWM0_CHANNEL0
		LPC_TMR16B0->PWMC |= 0b1;
		LPC_IOCON->PIO0_8 &= ~0b111;
		LPC_IOCON->PIO0_8 |= 0x2;
#endif
#if CONFIG_PWM0_CHANNEL1
		LPC_TMR16B0->PWMC |= 0b10;
		LPC_IOCON->PIO0_9 &= ~0b111;
		LPC_IOCON->PIO0_9 |= 0x2;
#endif
#if CONFIG_PWM0_CHANNEL2
		LPC_TMR16B0->PWMC |= 0b100;
		LPC_IOCON->PIO0_10 &= ~0b111;
		LPC_IOCON->PIO0_10 |= 0x3;
#endif
		break;
#endif
#if CONFIG_PWM1
	case PWM1:
#if CONFIG_PWM1_CHANNEL0
		LPC_TMR16B1->PWMC |= 0b1;
		LPC_IOCON->PIO1_9 &= ~0b111;
		LPC_IOCON->PIO1_9 |= 0x1;
#endif
#if CONFIG_PWM1_CHANNEL1
		LPC_TMR16B1->PWMC |= 0b10;
		LPC_IOCON->PIO1_10 &= ~0b111;
		LPC_IOCON->PIO1_10 |= 0x2;
#endif
#if CONFIG_PWM1_CHANNEL2
		LPC_TMR16B1->PWMC |= 0b100;
#endif
		break;
#endif
#if CONFIG_PWM2
	case PWM2:
#if CONFIG_PWM2_CHANNEL0
		LPC_TMR32B0->PWMC |= 0b1;
		LPC_IOCON->PIO1_6 &= ~0b111;
		LPC_IOCON->PIO1_6 |= 0x2;
#endif
#if CONFIG_PWM2_CHANNEL1
		LPC_TMR32B0->PWMC |= 0b10;
		LPC_IOCON->PIO1_7 &= ~0b111;
		LPC_IOCON->PIO1_7 |= 0x2;
#endif
#if CONFIG_PWM2_CHANNEL2
		LPC_TMR32B0->PWMC |= 0b100;
		LPC_IOCON->PIO0_1 &= ~0b111;
		LPC_IOCON->PIO0_1 |= 0x2;
#endif
		break;
#endif
#if CONFIG_PWM3
	case PWM3:
#if CONFIG_PWM3_CHANNEL0
		LPC_TMR32B1->PWMC |= 0b1;
		LPC_IOCON->PIO1_1 &= ~0b111;
		LPC_IOCON->PIO1_1 |= 0x3;
#endif
#if CONFIG_PWM3_CHANNEL1
		LPC_TMR32B1->PWMC |= 0b10;
		LPC_IOCON->PIO1_2 &= ~0b111;
		LPC_IOCON->PIO1_2 |= 0x3;
#endif
#if CONFIG_PWM3_CHANNEL2
		LPC_TMR32B1->PWMC |= 0b100;
		LPC_IOCON->PIO1_3 &= ~0b111;
		LPC_IOCON->PIO1_3 |= 0x3;
#endif
		break;
#endif
	default:
		break;
	}

	// set PWM's to zero
	this->timer[timer]->MR0 = 0;
	this->timer[timer]->MR1 = 0;
	this->timer[timer]->MR2 = 0;

	// clear tmr clear and stop for match 0, 1 and 2
	this->timer[timer]->MCR &= ~0x3FF;

	// reset timer on match 3
	this->timer[timer]->MCR |= (0b1 << 10);



	// start timer
	this->timer[timer]->TCR |= 1 << 0;

#elif CONFIG_TARGET_LPC1785
#warning "PWM initialization not yet implemented"

#endif
}
#endif

uw_errors_e uw_timer_init(uw_timers_e timer, float freq) {
	if (validate_timer(timer)) return validate_timer(timer);
	this_init(timer);
	SystemCoreClockUpdate();

	switch (timer) {
#if CONFIG_TIMER0
	case 0:
		init_timer(timer, freq);
		break;
#elif CONFIG_PWM0
	case 0:
		init_pwm(timer, freq);
		break;
#endif
#if CONFIG_TIMER1
	case 1:
		init_timer(timer, freq);
		break;
#elif CONFIG_PWM1
	case 1:
		init_pwm(timer, freq);
		break;
#endif
#if CONFIG_TIMER2
	case 2:
		init_timer(timer, freq);
		break;
#elif CONFIG_PWM2
	case 2:
		init_pwm(timer, freq);
		break;
#endif
#if CONFIG_TIMER3
	case 3:
		init_timer(timer, freq);
		break;
#elif CONFIG_PWM3
	case 3:
		init_pwm(timer, freq);
		break;
#endif
	default:
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}

	return uw_err(ERR_NONE);
}








uw_errors_e uw_timer_set_freq(uw_timers_e timer, float freq) {
	if (validate_timer(timer)) return validate_timer(timer);

	 SystemCoreClockUpdate();

	//set prescaler depending on in which mode init_value was given
	uint64_t prescaler;
#if CONFIG_TARGET_LPC11C14
	uint32_t max_value = 0xFFFF;
#if (CONFIG_TIMER2 || CONFIG_TIMER3)
	if (timer > TIMER1) {
		max_value = 0xFFFFFFFF;
	}
#endif
	prescaler = SystemCoreClock /
				(freq * max_value) - 1;
	// now we have prescaler when defined as a frequency
	// if the freq is too high for timer to calculate to it's max value,
	// decrease the max value accordingly
	if (prescaler == 0) {
		max_value = SystemCoreClock / freq;
	}
#elif CONFIG_TARGET_LPC1785
	uint32_t max_value = 0xFFFFFFFF;
	prescaler = PeripheralClock /
				( freq * max_value) - 1;
	if (prescaler == 0) {
		max_value = PeripheralClock / freq;
	}
#endif

	// add prescaler
	this->timer[timer]->PR = prescaler;

	// set match 3 to timer max value
	this->timer[timer]->MR3 = max_value;

	return uw_err(ERR_NONE);
}






uw_errors_e uw_pwm_set(uw_pwms_e pwm, uw_pwm_channels_e channel, unsigned int duty_cycle) {
#if CONFIG_TARGET_LPC11C14
	if (validate_timer(pwm)) return validate_timer(pwm);

	unsigned int max_value = this->timer[pwm]->MR3;
	// clamp duty cycle to max value
	if (duty_cycle > 1000) {
		duty_cycle = 1000;
	}
	// calculate register value from duty cycle
	unsigned int value = max_value / 1000 * (1000 - duty_cycle);
	switch(channel) {
	case PWM_CHANNEL_0:
		this->timer[pwm]->MR0 = value;
		break;
	case PWM_CHANNEL_1:
		this->timer[pwm]->MR1 = value;
		break;
	case PWM_CHANNEL_2:
		this->timer[pwm]->MR2 = value;
		break;
	default:
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}


	return uw_err(ERR_NONE);
#elif CONFIG_TARGET_LPC1785
#warning "uw_pwm_set for CONFIG_TARGET_LPC1785 is missing"
#endif
}




uw_errors_e uw_counter_init(uw_timers_e timer) {
	if (validate_timer(timer)) return validate_timer(timer);
	this_init(timer);
#if CONFIG_TARGET_LPC11C14
	__uw_err_throw(ERR_NOT_IMPLEMENTED | HAL_MODULE_TIMER);
#elif CONFIG_TARGET_LPC1785
#warning "uw_counter_init for CONFIG_TARGET_LPC1785 is missing"
	__uw_err_throw(ERR_NOT_IMPLEMENTED | HAL_MODULE_TIMER);
#endif

	return uw_err(ERR_NONE);
}



void uw_timer_start(uw_timers_e timer) {
	this->timer[timer]->TCR |= 1 << 0;
}





void uw_timer_stop(uw_timers_e timer) {
	this->timer[timer]->TCR &= ~(1);
}




void uw_timer_clear(uw_timers_e timer) {
	this->timer[timer]->PR = 0;
	this->timer[timer]->TC = 0;
}





int uw_timer_get_value(uw_timers_e timer) {
	return this->timer[timer]->TC;
}



uw_errors_e uw_timer_add_callback(uw_timers_e timer, void (*callback_function)
		(void* user_ptr, uw_timers_e timer, uw_timer_int_sources_e source, unsigned int value)) {
	if (validate_timer(timer)) return validate_timer(timer);

	// enable interrupts on this timer
	IRQn_Type tmr;
	switch (timer) {
#if CONFIG_TARGET_LPC11C14
#if (CONFIG_TIMER0 || CONFIG_COUNTER0)
	case 0: tmr = TIMER_16_0_IRQn; break;
#endif
#if (CONFIG_TIMER1 || CONFIG_COUNTER1)
	case 1: tmr = TIMER_16_1_IRQn; break;
#endif
#if (CONFIG_TIMER2 || CONFIG_COUNTER2)
	case 2: tmr = TIMER_32_0_IRQn; break;
#endif
#if (CONFIG_TIMER3 || CONFIG_COUNTER3)
	case 3: tmr = TIMER_32_1_IRQn; break;
#endif
#elif CONFIG_TARGET_LPC1785
#if (CONFIG_TIMER0 || CONFIG_COUNTER0)
	case 0: tmr = TIMER0_IRQn; break;
#endif
#if (CONFIG_TIMER1 || CONFIG_COUNTER1)
	case 1: tmr = TIMER1_IRQn; break;
#endif
#if (CONFIG_TIMER2 || CONFIG_COUNTER2)
	case 2: tmr = TIMER2_IRQn; break;
#endif
#if (CONFIG_TIMER3 || CONFIG_COUNTER3)
	case 3: tmr = TIMER3_IRQn; break;
#endif
#endif
	default:
		__uw_err_throw(ERR_HARDWARE_NOT_SUPPORTED | HAL_MODULE_TIMER);
	}
	NVIC_EnableIRQ(tmr);

	// remember callback function pointer
	this->timer_callbacks[timer] = callback_function;


	return uw_err(ERR_NONE);
}




#if !CONFIG_RTOS

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
	return uw_err(ERR_NONE);

}

void uw_tick_timer_add_callback(
		void (*callback_function_ptr)(void*, uint32_t)) {
	this->tick_callback = callback_function_ptr;
}

#endif
