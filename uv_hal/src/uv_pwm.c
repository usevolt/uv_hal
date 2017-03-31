/*
 * uv_pwm.c
 *
 *  Created on: Sep 19, 2016
 *      Author: usevolt
 */


#include "uv_pwm.h"


#if CONFIG_PWM

uv_errors_e _uv_pwm_init() {

#if CONFIG_TARGET_LPC1785
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

#elif CONFIG_TARGET_LPC11C14
#if CONFIG_PWM0_1
	LPC_IOCON->PIO0_8 = 0x2;
	LPC_TMR16B0->MR0 = 0;
#endif
#if CONFIG_PWM0_2
	LPC_IOCON->PIO0_9 = 0x2;
	LPC_TMR16B0->MR1 = 0;
#endif
#if CONFIG_PWM0_3
	LPC_IOCON->SWCLK_PIO0_10 = 0x3;
	LPC_TMR16B0->MR2 = 0;
#endif
#if CONFIG_PWM1_1
	LPC_IOCON->PIO1_9 = 0x1;
	LPC_TMR16B1->MR0 = 0;
#endif
#if CONFIG_PWM1_2
	LPC_IOCON->PIO1_10 = 0x2 | (1 << 7);
	LPC_TMR16B1->MR1 = 0;
#endif
#if CONFIG_PWM2_1
	LPC_IOCON->PIO1_6 = 0x2;
	LPC_TMR32B0->MR0 = 0;
#endif
#if CONFIG_PWM2_2
	LPC_IOCON->PIO1_7 = 0x2;
	LPC_TMR32B0->MR1 = 0;
#endif
#if CONFIG_PWM2_3
	LPC_IOCON->R_PIO0_11 = 0x3 | (1 << 7);
	LPC_TMR32B0->MR3 = 0;
#endif
#if CONFIG_PWM3_1
	LPC_IOCON->R_PIO1_1 = 0x3 | (1 << 7);
	LPC_TMR32B1->MR0 = 0;
#endif
#if CONFIG_PWM3_2
	LPC_IOCON->R_PIO1_2 = 0x3 | (1 << 7);
	LPC_TMR32B1->MR1 = 0;
#endif
#if CONFIG_PWM3_3
	LPC_IOCON->SWDIO_PIO1_3 = 0x3 | (1 << 7);
	LPC_TMR32B1->MR2 = 0;
#endif

#if CONFIG_PWM0
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 7);
	LPC_TMR16B0->TCR = 0x2;
	LPC_TMR16B0->PR = (SystemCoreClock / PWM_MAX_VALUE) / CONFIG_PWM_FREQ;
	LPC_TMR16B0->MCR = (1 << 10);
	LPC_TMR16B0->MR3 = PWM_MAX_VALUE;
	LPC_TMR16B0->CTCR = 0;
	LPC_TMR16B0->PWMC = (1 << 3);
#if CONFIG_PWM0_1
	LPC_TMR16B0->PWMC |= (1 << 0);
#endif
#if CONFIG_PWM0_2
	LPC_TMR16B0->PWMC |= (1 << 1);
#endif
#if CONFIG_PWM0_3
	LPC_TMR16B0->PWMC |= (1 << 2);
#endif
	LPC_TMR16B0->TCR = 0x1;
#endif
#if CONFIG_PWM1
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 8);

#endif
#if CONFIG_PWM2
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 9);

#endif
#if CONFIG_PWM3
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 10);

#endif

#endif

	return ERR_NONE;
}


#endif
