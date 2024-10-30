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


#include "uv_timer.h"


#include <stdlib.h>
#include <stdio.h>
#include "uv_utilities.h"
#include "chip.h"
#include "timer_17xx_40xx.h"

#if CONFIG_TIMER0 || CONFIG_TIMER1 || CONFIG_TIMER2 || CONFIG_TIMER3



static LPC_TIMER_T *timers[TIMER_COUNT] = {
		LPC_TIMER0,
		LPC_TIMER1,
		LPC_TIMER2,
		LPC_TIMER3
};


uv_errors_e uv_timer_init(uv_timers_e timer) {
	uv_errors_e ret = ERR_NONE;
	if (timer >= TIMER_COUNT) {
		ret = ERR_HARDWARE_NOT_SUPPORTED;
	}
	else {
		Chip_TIMER_Init(timers[timer]);
	}

	return ret;
}



void uv_timer_start(uv_timers_e timer) {
	Chip_TIMER_Enable(timers[timer]);
}





void uv_timer_stop(uv_timers_e timer) {
	Chip_TIMER_Disable(timers[timer]);
}




void uv_timer_clear(uv_timers_e timer) {
	timers[timer]->TC = 0;
}





uint32_t uv_timer_get_us(uv_timers_e timer) {
	uint32_t count = (uint64_t) Chip_TIMER_ReadCount(timers[timer]) * 1000000 /
			Chip_Clock_GetSystemClockRate();
	return count;
}



#endif

