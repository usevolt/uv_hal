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



