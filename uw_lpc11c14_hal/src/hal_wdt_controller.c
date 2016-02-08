/*
 * hal_wdt_controller.c
 *
 *  Created on: Apr 24, 2015
 *      Author: usevolt
 */



#include "LPC11xx.h"
#include "hal_wdt_controller.h"



void hal_init_wdt(unsigned int time_s, unsigned int fosc) {
	//enable clock to wdt
	LPC_SYSCON->SYSAHBCLKCTRL |= (1 << 15);
	//select IRC oscillator as the clock source
	LPC_SYSCON->WDTCLKSEL = 0x1;
	//divide clock source by 1, i.e. enable wdt clock source
	LPC_SYSCON->WDTCLKDIV = 4;

	//update changes to wdt clock settings by first writing a zero and then 1
	LPC_SYSCON->WDTCLKUEN = 0;
	LPC_SYSCON->WDTCLKUEN = 1;
	//set the reloading value
	///wdt has inner divide-by-4 prescaler
	unsigned int sck = time_s * (fosc / 16);
	//clamp cycle time to 24-bit value
	if (sck > 0xFFFFFF) {
		sck = 0xFFFFFF;
	}
	LPC_WDT->TC = sck;
	//enable the wdt
	LPC_WDT->MOD = 3;
	__disable_irq();
	LPC_WDT->FEED = 0xAA;
	LPC_WDT->FEED = 0x55;
	__enable_irq();

}


void hal_update_wdt(void) {
	__disable_irq();
	LPC_WDT->FEED = 0xAA;
	LPC_WDT->FEED = 0x55;
	__enable_irq();
}


void hal_reset_wdt(void) {
	__disable_irq();
	LPC_WDT->MOD = 3;
	LPC_WDT->FEED = 0xAA;
	//any oter write to wdt registers after FEED 0xAA will cause a wdt reset
	LPC_WDT->MOD = 3;
	__enable_irq();
}

