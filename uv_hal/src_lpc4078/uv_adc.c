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


/// @brief: Look up table defining all the adc channel ports, pins and the
/// IOCON function that connects the pin to the AD converter.
/// The adc channel enum can be used to index these.
///
/// @note: The function is not the same for every channel: AD0[0..3] sit on
/// P0[23..26] where the converter is the first alternate function, while
/// AD0[4..7] sit on P1[30], P1[31], P0[12] and P0[13] where it is the third.
static const struct {
	uint8_t port;
	uint8_t pin;
	uint8_t func;
} adc_table[ADC_COUNT - 1] = {
		{ 0, 23, IOCON_FUNC1 }, 	// ADC0_0
		{ 0, 24, IOCON_FUNC1 }, 	// ADC0_1
		{ 0, 25, IOCON_FUNC1 }, 	// ADC0_2
		{ 0, 26, IOCON_FUNC1 }, 	// ADC0_3
		{ 1, 30, IOCON_FUNC3 }, 	// ADC0_4
		{ 1, 31, IOCON_FUNC3 }, 	// ADC0_5
		{ 0, 12, IOCON_FUNC3 }, 	// ADC0_6
		{ 0, 13, IOCON_FUNC3 } 		// ADC0_7
};


/// @brief: How many times the conversion-complete flag is polled before a
/// reading is given up on. A conversion takes 65 AD clock cycles, i.e. a few
/// microseconds at any supported sample rate, so this only ever expires when
/// the converter is not running at all - it is here so that a misconfigured
/// channel returns an error instead of hanging the caller.
#define ADC_CONVERSION_TIMEOUT		10000

uv_errors_e _uv_adc_init() {

#if CONFIG_ADC0
	ADC_CLOCK_SETUP_T setup;
	// initialize ADC0
	Chip_ADC_Init(LPC_ADC, &setup);
#if CONFIG_ADC_CONVERSION_FREQ
	Chip_ADC_SetSampleRate(LPC_ADC, &setup, CONFIG_ADC_CONVERSION_FREQ);
#endif
#endif

	return ERR_NONE;
}



int16_t uv_adc_read(uv_adc_channels_e channel) {
	int16_t ret = -1;

#if CONFIG_ADC0
	// This chip has one converter with ADC_COUNT - 1 channels. The channel enum
	// is shared with targets that have more of them, so anything above this
	// converter's own channels has no data register to read and is rejected.
	if (channel != 0 &&
			channel < ADC_COUNT) {
		uint8_t ch = (uint8_t) (channel - 1);
		uint16_t value = 0;
		uint32_t timeout = ADC_CONVERSION_TIMEOUT;

		uv_adc_enable_ain(channel);

		// Software-controlled mode converts every channel selected in CR.SEL in
		// turn, so leaving other channels selected would spend conversions on
		// channels nobody asked for. Select exactly the one being read.
		// CR.SEL is the low 8 bits, one per channel.
		LPC_ADC->CR = (LPC_ADC->CR & ~0xFFUL) | ADC_CR_CH_SEL(ch);

		// Discard whatever the previous conversion left behind: the DONE flag
		// of an unread result would otherwise satisfy the wait below
		// immediately and hand back a stale reading.
		(void) LPC_ADC->DR[ch];

		Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);

		while ((Chip_ADC_ReadStatus(LPC_ADC, ch, ADC_DR_DONE_STAT) != SET) &&
				(timeout != 0)) {
			timeout--;
		}

		if ((timeout != 0) &&
				(Chip_ADC_ReadValue(LPC_ADC, ch, &value) == SUCCESS)) {
			ret = (int16_t) value;
		}
		else {
		}
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
		// Two things are needed to reach the converter, and PinMuxSet writes the
		// whole IOCON register: the pin's analog mode (MD_ANA_ENA clears the
		// digital-mode bit) and the pin function that routes it to the AD
		// converter. MD_ANA_ENA on its own leaves the function at 0, which is
		// plain GPIO, and the channel then reads nothing.
		Chip_IOCON_PinMuxSet(LPC_IOCON,
				adc_table[channel - 1].port,
				adc_table[channel - 1].pin,
				adc_table[channel - 1].func | MD_ANA_ENA);
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
