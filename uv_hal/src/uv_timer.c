/*
 * uv_timer_controller.c
 *
 *  Created on: Jan 28, 2015
 *      Author: usenius
 */


#include "uv_timer.h"


#include <stdlib.h>
#include <stdio.h>
#include "uv_utilities.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif

typedef struct {

} this_st;

this_st _this;
#define this (&_this)



// timer interrupt handler

static void parse_timer_interrupt(uv_timers_e timer) {

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


static void init_timer(unsigned int timer, unsigned int freq) {

}


uv_errors_e uv_timer_init(uv_timers_e timer, float freq) {

	return uv_err(ERR_NONE);
}








uv_errors_e uv_timer_set_freq(uv_timers_e timer, float freq) {

	return uv_err(ERR_NONE);
}








uv_errors_e uv_counter_init(uv_timers_e timer) {

	return uv_err(ERR_NONE);
}



void uv_timer_start(uv_timers_e timer) {
}





void uv_timer_stop(uv_timers_e timer) {
}




void uv_timer_clear(uv_timers_e timer) {
}





int uv_timer_get_value(uv_timers_e timer) {
	return 0;
}





