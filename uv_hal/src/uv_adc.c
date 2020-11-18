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
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#elif CONFIG_TARGET_LPC1549
#include "chip.h"
#include "adc_15xx.h"
#include "uv_gpio.h"
#endif


typedef struct {
#if CONFIG_TARGET_LPC1549
	uint16_t adc0_channels;
	uint16_t adc1_channels;
#endif
} adc_st;

static volatile adc_st adc;
#define this (&adc)

#if CONFIG_ADC || CONFIG_ADC0 || CONFIG_ADC1



uv_errors_e _uv_adc_init() {

	this->adc0_channels = 0;
	this->adc1_channels = 0;


#if CONFIG_ADC0
	// initialize ADC0
	Chip_ADC_Init(LPC_ADC0, 0);
	Chip_ADC_SetClockRate(LPC_ADC0, CONFIG_ADC_CONVERSION_FREQ * 25);
	Chip_ADC_SetTrim(LPC_ADC0, ADC_TRIM_VRANGE_HIGHV);
	Chip_ADC_StartCalibration(LPC_ADC0);
	while (!Chip_ADC_IsCalibrationDone(LPC_ADC0));
	Chip_ADC_SetClockRate(LPC_ADC0, CONFIG_ADC_CONVERSION_FREQ * 25);
#if (CONFIG_ADC_MODE == ADC_MODE_ASYNC)
	Chip_ADC_EnableInt(LPC_ADC0, ADC_INTEN_SEQA_ENABLE);
	NVIC_EnableIRQ(ADC0_SEQA_IRQn);
#endif

#if CONFIG_ADC_CHANNEL0_0
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_0);
#endif
#if CONFIG_ADC_CHANNEL0_1
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 7,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_1);
#endif
#if CONFIG_ADC_CHANNEL0_2
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 6,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_2);
#endif
#if CONFIG_ADC_CHANNEL0_3
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_3);
#endif
#if CONFIG_ADC_CHANNEL0_4
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_4);
#endif
#if CONFIG_ADC_CHANNEL0_5
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 3,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_5);
#endif
#if CONFIG_ADC_CHANNEL0_6
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 2,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_6);
#endif
#if CONFIG_ADC_CHANNEL0_7
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_7);
#endif
#if CONFIG_ADC_CHANNEL0_8
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 0,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_8);
#endif
#if CONFIG_ADC_CHANNEL0_9
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 31,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_9);
#endif
#if CONFIG_ADC_CHANNEL0_10
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 0,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_10);
#endif
#if CONFIG_ADC_CHANNEL0_11
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 30,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC0_11);
#endif

#if CONFIG_ADC1

	// initialize ADC1
	Chip_ADC_Init(LPC_ADC1, 0);
	Chip_ADC_SetClockRate(LPC_ADC1, CONFIG_ADC_CONVERSION_FREQ * 25);
	Chip_ADC_SetTrim(LPC_ADC1, ADC_TRIM_VRANGE_HIGHV);
	Chip_ADC_StartCalibration(LPC_ADC1);
	while (!Chip_ADC_IsCalibrationDone(LPC_ADC1));
	Chip_ADC_SetClockRate(LPC_ADC1, CONFIG_ADC_CONVERSION_FREQ * 25);
#if (CONFIG_ADC_MODE == ADC_MODE_ASYNC)
	Chip_ADC_EnableInt(LPC_ADC1, ADC_INTEN_SEQA_ENABLE);
	NVIC_EnableIRQ(ADC1_SEQA_IRQn);
#endif

#if CONFIG_ADC_CHANNEL1_0
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 1,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_0);
#endif
#if CONFIG_ADC_CHANNEL1_1
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_1);
#endif
#if CONFIG_ADC_CHANNEL1_2
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 10,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_2);
#endif
#if CONFIG_ADC_CHANNEL1_3
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 11,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_3);
#endif
#if CONFIG_ADC_CHANNEL1_4
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 2,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_4);
#endif
#if CONFIG_ADC_CHANNEL1_5
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 3,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_5);
#endif
#if CONFIG_ADC_CHANNEL1_6
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_6);
#endif
#if CONFIG_ADC_CHANNEL1_7
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_7);
#endif
#if CONFIG_ADC_CHANNEL1_8
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 15,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_8);
#endif
#if CONFIG_ADC_CHANNEL1_9
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_9);
#endif
#if CONFIG_ADC_CHANNEL1_10
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 4,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_10);
#endif
#if CONFIG_ADC_CHANNEL1_11
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 5,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_11);
#endif

#endif

#endif

	return ERR_NONE;
}



int16_t uv_adc_read(uv_adc_channels_e channel) {
	int16_t ret = -1;


#if CONFIG_ADC0
	// channel 1
	if (channel < (1 << 12)) {
		uv_disable_int();
		Chip_ADC_DisableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX,
				channel | ADC_SEQ_CTRL_HWTRIG_POLPOS);
		for (uint16_t i = 0; i < 0x800; i++) {
			__NOP();
		}
		Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX);
		// wait for the conversion to finish
		while (!(LPC_ADC0->SEQ_GDAT[ADC_SEQA_IDX] & (1 << 31)));
		uint32_t raw = Chip_ADC_GetDataReg(LPC_ADC0, uv_ctz(channel));
		uv_enable_int();
		ret = ADC_DR_RESULT(raw);
	}
#endif
#if CONFIG_ADC1
	// channel 2
	if (channel >= (1 << 12)) {
		channel = channel >> 12;
		uv_disable_int();
		Chip_ADC_DisableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC1, ADC_SEQA_IDX,
				channel | ADC_SEQ_CTRL_HWTRIG_POLPOS);
		Chip_ADC_EnableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC1, ADC_SEQA_IDX);
		// wait for the conversion to finish
		while (!(LPC_ADC1->SEQ_GDAT[ADC_SEQA_IDX] & (1 << 31)));
		uint32_t raw = Chip_ADC_GetDataReg(LPC_ADC1, uv_ctz(channel));
		uv_enable_int();
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
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 3,
			(IOCON_MODE_INACT | IOCON_ADMODE_EN));
	Chip_SWM_EnableFixedPin(SWM_FIXED_ADC1_5);
}



void uv_adc_disable_ain(uv_adc_channels_e channel) {

}



#endif
