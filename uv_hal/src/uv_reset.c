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




#include "uv_reset.h"
#include <stdlib.h>

#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#elif CONFIG_TARGET_LPC1549
#include "chip.h"
#endif
#include "uv_wdt.h"
#include "uv_memory.h"
#include <string.h>
#include <uv_rtos.h>


void uv_system_reset() {
#if CONFIG_UV_BOOTLOADER
	// if bootloader is enabled, make sure bootloader shared memory is cleared
	memset(UV_BOOTLOADER_DATA_ADDR, 0, UV_BOOTLOADER_DATA_LEN);
#endif
	uv_bootloader_start();
}



void uv_bootloader_start() {
#if !CONFIG_TARGET_LINUX && !CONFIG_TARGET_WIN
	// BUGFIX NOTE: When resetting, SCT PWM outputs are left ON, keeping
	// those pins pulled low. To prevent this, SWM mappings from SCT timers
	// will be cleared here. Some testing with SYSCON periph resets didn't work...
	LPC_SWM->PINASSIGN[7] |= 0x00FFFFFF;
	LPC_SWM->PINASSIGN[8] = 0xFFFFFFFF;
	LPC_SWM->PINASSIGN[9] = 0xFFFFFFFF;
	LPC_SWM->PINASSIGN[10] |= 0xFF;

	NVIC_SystemReset();
#else
	exit(0);
#endif
}



