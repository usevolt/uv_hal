/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#include "uv_wdt.h"

#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif

#if CONFIG_WDT


void _uv_wdt_init(void) {
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
	uint32_t sck = CONFIG_WDT_CYCLE_S * (SystemCoreClock / 16);
#elif CONFIG_TARGET_LPC1785
	uint32_t sck = CONFIG_WDT_CYCLE_S * (500000 / 4);
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

#endif
