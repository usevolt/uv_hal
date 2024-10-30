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

#include "uv_adc.h"

#include <stdio.h>
#include "uv_uart.h"
#include "uv_rtos.h"
#include "chip.h"
#include "adc_17xx_40xx.h"
#include "uv_gpio.h"



#if CONFIG_ADC || CONFIG_ADC0


/// @brief: Look up table defining all the adc channel ports and pins.
/// The adc channel enum can be used to index these.
static const struct {
	uint8_t port;
	uint8_t pin;
} adc_table[ADC_COUNT - 1] = {
		{ 0, 23 }, 	// ADC0_0
		{ 0, 24 }, 	// ADC0_1
		{ 0, 25 }, 	// ADC0_2
		{ 0, 26 }, 	// ADC0_3
		{ 1, 30 }, 	// ADC0_4
		{ 1, 31 }, 	// ADC0_5
		{ 0, 12 }, 	// ADC0_6
		{ 0, 13 } 	// ADC0_7
};

uv_errors_e _uv_adc_init() {

#if CONFIG_ADC0
	// initialize ADC0
	Chip_ADC_Init(LPC_ADC, 0);

#endif

	return ERR_NONE;
}



int16_t uv_adc_read(uv_adc_channels_e channel) {
	int16_t ret = -1;

	uv_adc_enable_ain(channel);

#if CONFIG_ADC0
	// channel 1
	if (channel != 0 &&
			channel < ADC1_0) {
		Chip_ADC_ReadValue(LPC_ADC, channel - 1, (uint16_t*) &ret);
	}
#endif

	return ret;
}






int16_t uv_adc_read_average(uv_adc_channels_e channel, uint32_t conversion_count) {
	int32_t value = 0, i;
	for (i = 0; i < conversion_count; i++) {
		value += uv_adc_read(channel);
	}
	value /= conversion_count;
	return (int16_t) value;
}




void uv_adc_enable_ain(uv_adc_channels_e channel) {
	if (channel != 0 &&
			channel < ADC_COUNT) {
		Chip_IOCON_PinMuxSet(LPC_IOCON,
				adc_table[channel - 1].port,
				adc_table[channel - 1].pin,
				MD_ANA_ENA);
		Chip_ADC_EnableChannel(LPC_ADC, channel - 1, ENABLE);
	}
}



void uv_adc_disable_ain(uv_adc_channels_e channel) {
	if (channel != 0 &&
			channel < ADC_COUNT) {
		Chip_IOCON_PinMuxSet(LPC_IOCON,
				adc_table[channel - 1].port,
				adc_table[channel - 1].pin,
				MD_ANA_DIS);
		Chip_ADC_EnableChannel(LPC_ADC, channel - 1, DISABLE);
	}
}



uv_gpios_e uv_adc_get_gpio_pin(uv_adc_channels_e channel) {
	uint32_t ret = 0;
	if (channel != 0 &&
			channel < ADC_COUNT) {
		ret = 32 * adc_table[channel - 1].port + adc_table[channel - 1].pin + 1;
	}
	return ret;
}



#endif
