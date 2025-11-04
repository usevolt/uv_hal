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
#if CONFIG_TARGET_LPC15XX
#include "chip.h"
#include "adc_15xx.h"
#include "uv_gpio.h"
#endif



#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1


/// @brief: Look up table defining all the adc channel ports and pins.
/// The adc channel enum can be used to index these.
static const struct {
	uint8_t port;
	uint8_t pin;
} adc_table[ADC_COUNT - 1] = {
		{ 0, 8 }, 	// ADC0_0
		{ 0, 7 }, 	// ADC0_1
		{ 0, 6 }, 	// ADC0_2
		{ 0, 5 }, 	// ADC0_3
		{ 0, 4 }, 	// ADC0_4
		{ 0, 3 }, 	// ADC0_5
		{ 0, 2 }, 	// ADC0_6
		{ 0, 1 }, 	// ADC0_7
		{ 1, 0 }, 	// ADC0_8
		{ 0, 31 }, 	// ADC0_9
		{ 0, 0 }, 	// ADC0_10
		{ 0, 30 }, 	// ADC0_11
		{ 1, 1 }, 	// ADC1_0
		{ 0, 9 }, 	// ADC1_1
		{ 0, 10 }, 	// ADC1_2
		{ 0, 11 }, 	// ADC1_3
		{ 1, 2 }, 	// ADC1_4
		{ 1, 3 }, 	// ADC1_5
		{ 0, 13 }, 	// ADC1_6
		{ 0, 14 }, 	// ADC1_7
		{ 0, 15 }, 	// ADC1_8
		{ 0, 16 }, 	// ADC1_9
		{ 1, 4 }, 	// ADC1_10
		{ 1, 5 } 	// ADC1_11
};

uv_errors_e _uv_adc_init() {

#if CONFIG_ADC0
	// initialize ADC0
	Chip_ADC_Init(LPC_ADC0, 0);
	Chip_ADC_SetClockRate(LPC_ADC0, CONFIG_ADC_CONVERSION_FREQ * 25);
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV);
	Chip_ADC_StartCalibration(LPC_ADC0);
	while (!Chip_ADC_IsCalibrationDone(LPC_ADC0));
	Chip_ADC_SetClockRate(LPC_ADC0, CONFIG_ADC_CONVERSION_FREQ * 25);

#if CONFIG_ADC1

	// initialize ADC1
	Chip_ADC_Init(LPC_ADC1, 0);
	Chip_ADC_SetClockRate(LPC_ADC1, CONFIG_ADC_CONVERSION_FREQ * 25);
	Chip_ADC_SetTrim(LPC_ADC1, ADC_TRIM_VRANGE_HIGHV);
	Chip_ADC_StartCalibration(LPC_ADC1);
	while (!Chip_ADC_IsCalibrationDone(LPC_ADC1));
	Chip_ADC_SetClockRate(LPC_ADC1, CONFIG_ADC_CONVERSION_FREQ * 25);

#endif

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
		Chip_ADC_DisableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX,
				ADC_SEQ_CTRL_CHANSEL(channel - 1) | ADC_SEQ_CTRL_HWTRIG_POLPOS);
		// for some reason LPC1549 needs a bit time for ADC1 sequencer to setup.
		// 24.10.2025 __NOP() command changed to freertos yield so that
		// other tasks dont interrupt because of this
		for (uint16_t i = 0; i < 0x400; i++) {
			uv_rtos_task_yield();
		}
		Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX);
		// wait for the conversion to finish
		while (!(LPC_ADC0->SEQ_GDAT[ADC_SEQA_IDX] & (1 << 31))) {
			uv_rtos_task_yield();
		}
		uint32_t raw = Chip_ADC_GetDataReg(LPC_ADC0, channel - 1);
		ret = ADC_DR_RESULT(raw);
	}
#endif
#if CONFIG_ADC1
	// channel 2
	if (channel >= ADC1_0 && channel < ADC_COUNT) {
		channel -= ADC1_0 - 1;
		Chip_ADC_DisableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC1, ADC_SEQA_IDX,
				ADC_SEQ_CTRL_CHANSEL(channel - 1) | ADC_SEQ_CTRL_HWTRIG_POLPOS);
		Chip_ADC_EnableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC1, ADC_SEQA_IDX);
		// wait for the conversion to finish
		while (!(LPC_ADC1->SEQ_GDAT[ADC_SEQA_IDX] & (1 << 31))) {
			uv_rtos_task_yield();
		}
		uint32_t raw = Chip_ADC_GetDataReg(LPC_ADC1, channel - 1);
		ret = ADC_DR_RESULT(raw);
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
		Chip_IOCON_PinMuxSet(LPC_IOCON, adc_table[channel - 1].port, adc_table[channel - 1].pin,
				(IOCON_MODE_INACT | IOCON_ADMODE_EN));
		// depends on the SWM_FIXED_ADCX_X values to be in ascending order
		Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_0 + channel - 1);
	}
}



void uv_adc_disable_ain(uv_adc_channels_e channel) {
	if (channel != 0 &&
			channel < ADC_COUNT) {
		// depends on the SWM_FIXED_ADCX_X values to be in ascending order
		Chip_SWM_DisableFixedPin(SWM_FIXED_ADC0_0 + channel - 1);
		Chip_IOCON_PinMuxSet(LPC_IOCON, adc_table[channel - 1].port, adc_table[channel - 1].pin,
				(IOCON_MODE_INACT | IOCON_HYS_EN | IOCON_DIGMODE_EN));
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
