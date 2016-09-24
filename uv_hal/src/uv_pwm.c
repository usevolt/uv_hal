/*
 * uv_pwm.c
 *
 *  Created on: Sep 19, 2016
 *      Author: usevolt
 */


#include "uv_pwm.h"

uv_errors_e uv_pwm_init() {
	// enable power
#if CONFIG_PWM0
	LPC_SC->PCONP |= (1 << 5);
#endif
#if CONFIG_PWM1
	LPC_SC->PCONP |= (1 << 6);
#endif
	// set the clock divider if not already set
	if (!(LPC_SC->PCLKSEL)) {
		LPC_SC->PCLKSEL = 1;
	}
#if CONFIG_PWM0_1
	LPC_IOCON->P1_2 = 0b011;
	LPC_PWM0->MR1 = 0;
	LPC_PWM0->PCR |= (1 << 9);
#endif
#if CONFIG_PWM0_2
	LPC_IOCON->P1_3 = 0b011;
	LPC_PWM0->MR2 = 0;
	LPC_PWM0->PCR |= (1 << 10);
#endif
#if CONFIG_PWM0_3
	LPC_IOCON->P1_5 = 0b011;
	LPC_PWM0->MR3 = 0;
	LPC_PWM0->PCR |= (1 << 11);
#endif
#if CONFIG_PWM0_4
	LPC_IOCON->P1_6 = 0b011;
	LPC_PWM0->MR4 = 0;
	LPC_PWM0->PCR |= (1 << 12);
#endif
#if CONFIG_PWM0_5
	LPC_IOCON->P1_7 = 0b011;
	LPC_PWM0->MR5 = 0;
	LPC_PWM0->PCR |= (1 << 13);
#endif
#if CONFIG_PWM0_6
	LPC_IOCON->P1_11 = 0b011;
	LPC_PWM0->MR6 = 0;
	LPC_PWM0->PCR |= (1 << 14);
#endif
#if CONFIG_PWM1_1
	LPC_IOCON->P1_18 = 0b011;
	LPC_PWM1->MR1 = 0;
	LPC_PWM1->PCR |= (1 << 9);
#endif
#if CONFIG_PWM1_2
	LPC_IOCON->P1_20 = 0b011;
	LPC_PWM1->MR2 = 0;
	LPC_PWM1->PCR |= (1 << 10);
#endif
#if CONFIG_PWM1_3
	LPC_IOCON->P1_21 = 0b011;
	LPC_PWM1->MR3 = 0;
	LPC_PWM1->PCR |= (1 << 11);
#endif
#if CONFIG_PWM1_4
	LPC_IOCON->P1_23 = 0b011;
	LPC_PWM1->MR4 = 0;
	LPC_PWM1->PCR |= (1 << 12);
#endif
#if CONFIG_PWM1_5
	LPC_IOCON->P1_24 = 0b011;
	LPC_PWM1->MR5 = 0;
	LPC_PWM1->PCR |= (1 << 13);
#endif
#if CONFIG_PWM1_6
	LPC_IOCON->P1_26 = 0b011;
	LPC_PWM1->MR6 = 0;
	LPC_PWM1->PCR |= (1 << 14);
#endif

#if CONFIG_PWM0
	// set match 0 to reset the timer
	LPC_PWM0->MCR = (1 << 1);
	LPC_PWM0->MR0 = PWM_MAX_VALUE;
	LPC_PWM0->LER = 0x7F;
	// set prescaler
	LPC_PWM0->PR = (SystemCoreClock / PWM_MAX_VALUE) / CONFIG_PWM_FREQ;
	// make sure timer mode is selected
	LPC_PWM0->CTCR = 0;
	// enable PWM mode and start counting
	LPC_PWM0->TCR |= (1 << 3) | (1 << 0);
#endif
#if CONFIG_PWM1
	LPC_PWM1->TCR = 2;	// timer reset
	LPC_PWM1->MR0 = PWM_MAX_VALUE;
	LPC_PWM1->LER = 63;
	LPC_PWM1->MCR = (1 << 1);
	LPC_PWM1->PR = (SystemCoreClock / PWM_MAX_VALUE) / CONFIG_PWM_FREQ;
	LPC_PWM1->CTCR = 0;
	LPC_PWM1->TCR |= (1 << 3) | (1 << 0);
#endif


	return uv_err(ERR_NONE);
}


