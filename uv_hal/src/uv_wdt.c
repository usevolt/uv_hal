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

#if CONFIG_WDT
#include "chip.h"
#include "wwdt_15xx.h"
#include <stdio.h>



void _uv_wdt_init(void) {
	SystemCoreClockUpdate();

	__disable_irq();

	/* Enable the WDT oscillator */
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_WDTOSC_PD);

	/* The WDT divides the input frequency into it by 4 */
	int32_t wdtFreq = Chip_Clock_GetWDTOSCRate() / 4;

	/* Initialize WWDT (also enables WWDT clock) */
	Chip_WWDT_Init(LPC_WWDT);

	Chip_WWDT_SetTimeOut(LPC_WWDT, wdtFreq * CONFIG_WDT_CYCLE_S);
	Chip_WWDT_SetOption(LPC_WWDT, WWDT_WDMOD_WDRESET);

	Chip_WWDT_Start(LPC_WWDT);

	__enable_irq();
}

void uv_wdt_update(void) {
	__disable_irq();
	Chip_WWDT_Feed(LPC_WWDT);
	__enable_irq();
}



#else

void uv_wdt_update(void) {

}

#endif
