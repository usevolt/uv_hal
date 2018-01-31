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


#include "uv_timer.h"


#include <stdlib.h>
#include <stdio.h>
#include "uv_utilities.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#elif CONFIG_TARGET_LPC1549
#include "chip.h"
#include "sct_15xx.h"
#endif

#if CONFIG_TIMER0 || CONFIG_TIMER1 || CONFIG_TIMER2 || CONFIG_TIMER3
typedef struct {

} timers_st;

timers_st _timers;
#define this (&_timers)



#if CONFIG_TARGET_LPC1549
LPC_SCT_T *timers[TIMER_COUNT] = {
		LPC_SCT0,
		LPC_SCT1,
		LPC_SCT2,
		LPC_SCT3
};
#endif


uv_errors_e uv_timer_init(uv_timers_e timer) {
	uv_errors_e ret = ERR_NONE;
	if (timer >= TIMER_COUNT) {
		ret = ERR_HARDWARE_NOT_SUPPORTED;
	}
	else {
		Chip_SCT_Init(timers[timer]);
		Chip_SCT_Config(timers[timer], SCT_CONFIG_32BIT_COUNTER);
	}

	return ret;
}



void uv_timer_start(uv_timers_e timer) {
	Chip_SCT_ClearControl(timers[timer], SCT_CTRL_HALT_L | SCT_CTRL_STOP_L);
}





void uv_timer_stop(uv_timers_e timer) {
	Chip_SCT_SetControl(timers[timer], SCT_CTRL_HALT_L);
}




void uv_timer_clear(uv_timers_e timer) {
	bool halt = timers[timer]->CTRL_U & (SCT_CTRL_HALT_L);
	Chip_SCT_SetControl(timers[timer], SCT_CTRL_HALT_L);
	Chip_SCT_SetCount(timers[timer], 0);
	if (!halt) {
		Chip_SCT_ClearControl(timers[timer], SCT_CTRL_HALT_L);
	}
}





int32_t uv_timer_get_us(uv_timers_e timer) {
	uint32_t count = (uint64_t) timers[timer]->COUNT_U * 1000000 /
			Chip_Clock_GetSystemClockRate();
	return count;
}



#endif

