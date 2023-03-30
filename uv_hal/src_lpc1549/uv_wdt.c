/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "uv_wdt.h"


#include "chip.h"
#include "wwdt_15xx.h"
#include <stdio.h>



void _uv_wdt_init(void) {
#if CONFIG_WDT
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
#endif
}

void uv_wdt_update(void) {
#if CONFIG_WDT
	__disable_irq();
	Chip_WWDT_Feed(LPC_WWDT);
	__enable_irq();
#endif
}



