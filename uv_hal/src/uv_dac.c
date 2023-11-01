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



#include "uv_dac.h"

#if CONFIG_TARGET_LPC15XX
#include "chip.h"
#include "dac_15xx.h"
#endif

#if CONFIG_TARGET_LPC15XX

void _uv_dac_init(void) {

#if CONFIG_DAC
	Chip_DAC_Init(LPC_DAC);
	// disable DAC, as it is enabled by default
	Chip_SWM_EnableFixedPin(SWM_FIXED_DAC_OUT);
#else
	// disable DAC, as it is enabled by default
	Chip_SWM_DisableFixedPin(SWM_FIXED_DAC_OUT);

#endif
}

#else
void _uv_dac_init(void) {
}

#endif

#if CONFIG_DAC

#if CONFIG_TARGET_LPC15XX


void uv_dac_set(uint32_t value) {
	if (value > DAC_MAX_VALUE) {
		value = DAC_MAX_VALUE;
	}
	Chip_DAC_UpdateValue(LPC_DAC, value);
}


#endif

#endif
