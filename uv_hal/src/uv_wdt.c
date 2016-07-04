/*
 * hal_wdt_controller.c
 *
 *  Created on: Apr 24, 2015
 *      Author: usevolt
 */


#include "uv_wdt.h"

#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif



void uv_wdt_init(unsigned int time_s) {
	SystemCoreClockUpdate();
#if CONFIG_TARGET_LPC11C14
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
	unsigned int sck = time_s * (SystemCoreClock / 16);
#elif CONFIG_TARGET_LPC1785
	unsigned int sck = time_s * (500000 / 4);
#endif
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

void uv_wdt_update(void) {
	__disable_irq();
	LPC_WDT->FEED = 0xAA;
	LPC_WDT->FEED = 0x55;
	__enable_irq();
}


void uv_wdt_reset(void) {
	__disable_irq();
	LPC_WDT->MOD = 3;
	LPC_WDT->FEED = 0xAA;
	//any otHer write to wdt registers after FEED 0xAA will cause a wdt reset
	LPC_WDT->MOD = 3;
	__enable_irq();
}

