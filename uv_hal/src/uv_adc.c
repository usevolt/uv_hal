/*
 * uv_adc_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
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


#if (CONFIG_ADC_MODE == ADC_MODE_ASYNC)
extern void CONFIG_ADC_CALLBACK (void);
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

#if CONFIG_TARGET_LPC11C14
///controller specific initializations for AD pins
//set pin as analog pin, disable pull up resistor and set to analog mode
#define ADC_CHN_0_INIT		LPC_IOCON->R_PIO0_11 |= 0x02; \
							LPC_IOCON->R_PIO0_11 &= ~((1 << 7) | (0b11 << 3))

#define ADC_CHN_1_INIT		LPC_IOCON->R_PIO1_0 |= 0x02; \
							LPC_IOCON->R_PIO1_0 &= ~((1 << 7) | (0b11 << 3))

#define ADC_CHN_2_INIT		LPC_IOCON->R_PIO1_1 |= 0x02; \
							LPC_IOCON->R_PIO1_1 &= ~((1 << 7) | (0b11 << 3))

#define ADC_CHN_3_INIT		LPC_IOCON->R_PIO1_2 |= 0x02; \
							LPC_IOCON->R_PIO1_2 &= ~((1 << 7) | (0b11 << 3))

#define ADC_CHN_4_INIT		LPC_IOCON->SWDIO_PIO1_3 |= 0x02; \
							LPC_IOCON->SWDIO_PIO1_3 &= ~(1 << 7)

#define ADC_CHN_5_INIT		LPC_IOCON->PIO1_4 |= 0x01; \
							LPC_IOCON->PIO1_4 &= ~((1 << 7) | (0b11 << 3))

#define ADC_CHN_6_INIT		LPC_IOCON->PIO1_10 |= 0x01; \
							LPC_IOCON->PIO1_10 &= ~((1 << 7) | (0b11 << 3))

#define ADC_CHN_7_INIT		LPC_IOCON->PIO1_11 |= 0x01; \
							LPC_IOCON->PIO1_11 &= ~((1 << 7) | (0b11 << 3))
#endif



uv_errors_e _uv_adc_init() {
	this->adc0_channels = 0;
	this->adc1_channels = 0;


#if CONFIG_TARGET_LPC11C14

#if CONFIG_ADC_CHANNEL0
	ADC_CHN_0_INIT;
#endif
#if CONFIG_ADC_CHANNEL1
	ADC_CHN_1_INIT;
#endif
#if CONFIG_ADC_CHANNEL2
	ADC_CHN_2_INIT;
#endif
#if CONFIG_ADC_CHANNEL3
	ADC_CHN_3_INIT;
#endif
#if CONFIG_ADC_CHANNEL4
	ADC_CHN_4_INIT;
#endif
#if CONFIG_ADC_CHANNEL5
	ADC_CHN_5_INIT;
#endif
#if CONFIG_ADC_CHANNEL6
	ADC_CHN_6_INIT;
#endif
#if CONFIG_ADC_CHANNEL7
	ADC_CHN_7_INIT;
#endif

	//enable clock to the adc
	LPC_SYSCON->SYSAHBCLKCTRL |= 1 << 13;
	//power on the adc
	LPC_SYSCON->PDRUNCFG &= ~(1 << 4);

	SystemCoreClockUpdate();

	//set clock divider
	//divide system clock by desired conversion speed. Max speed is 4.5 MHz.
	//mask off irrelevant bits, then left-shift the result to CLKDIV place.
	LPC_ADC->CR |= ((SystemCoreClock / 9000000) & 0xFF) << 8;

	//set adc to software controlled mode
	LPC_ADC->CR &= ~(1 << 16);

#elif CONFIG_TARGET_LPC1785

#if CONFIG_ADC_CHANNEL0
	ADC_0_INIT;
#endif
#if CONFIG_ADC_CHANNEL1
	ADC_1_INIT;
#endif
#if CONFIG_ADC_CHANNEL2
	ADC_2_INIT;
#endif
#if CONFIG_ADC_CHANNEL3
	ADC_3_INIT;
#endif
#if CONFIG_ADC_CHANNEL4
	ADC_4_INIT;
#endif
#if CONFIG_ADC_CHANNEL5
	ADC_5_INIT;
#endif
#if CONFIG_ADC_CHANNEL6
	ADC_6_INIT;
#endif
#if CONFIG_ADC_CHANNEL7
	ADC_7_INIT;
#endif

	//enable clock to the adc
	LPC_SC->PCONP |= (1 << 12);

	SystemCoreClockUpdate();

	//set clock divider
	//divide system clock by desired conversion speed. Max speed is 12.4 MHz.
	//mask off irrelevant bits, then left-shift the result to CLKDIV place.
	LPC_ADC->CR |= (((SystemCoreClock / 12000000) + 1) & 0xFF) << 8;

	//set adc to software controlled mode
	LPC_ADC->CR &= ~(1 << 16);
	// dont start ADC now
	LPC_ADC->CR &= ~(0b111 << 24);
	// ADC ON
	LPC_ADC->CR|= (1 << 21);

#elif CONFIG_TARGET_LPC1549

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

#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785

	// check if the channel is valid
	// LPC11C22 has 8 channels, so channel has to be less than 8
	if ((channel & (channel - 1)) == 0) {
		int16_t value = 0;
		// make sure that only one channel is selected
		//set the channel
		LPC_ADC->CR &= ~(0xFF);
		LPC_ADC->CR |= channel & 0xFF;
		// start the conversion
		LPC_ADC->CR &= ~(0b111 << 24);
		LPC_ADC->CR |= (1 << 24);
		//wait until the conversion is finished
		while (!(LPC_ADC->STAT & (1 << 16))) {
			uv_rtos_task_yield();
		}
		//read the acquired value
#if CONFIG_TARGET_LPC11C14
		value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 6) & 0x3FF;
#elif CONFIG_TARGET_LPC1785
		value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 4) & 0xFFF;
#endif
		ret = value;
	}

#elif CONFIG_TARGET_LPC1549

#if CONFIG_ADC0
	// channel 1
	if (channel < (1 << 12)) {
#if (CONFIG_ADC_MODE == ADC_MODE_SYNC)
		Chip_ADC_DisableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX,
				channel | ADC_SEQ_CTRL_HWTRIG_POLPOS);
		for (uint16_t i = 0; i < 0x800; i++) {
			__NOP();
		}
		Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX);
		// wait for the conversion to finish
		while (!(LPC_ADC0->SEQ_GDAT[ADC_SEQA_IDX] & (1 << 31))) {
			uv_rtos_task_yield();
		}
#endif
		uint32_t raw = Chip_ADC_GetDataReg(LPC_ADC0, uv_ctz(channel));
		ret = ADC_DR_RESULT(raw);
	}
#endif
#if CONFIG_ADC1
	// channel 2
	if (channel >= (1 << 12)) {
		channel = channel >> 12;
#if (CONFIG_ADC_MODE == ADC_MODE_SYNC)
		Chip_ADC_DisableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC1, ADC_SEQA_IDX,
				channel | ADC_SEQ_CTRL_HWTRIG_POLPOS);
		Chip_ADC_EnableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC1, ADC_SEQA_IDX);
		// wait for the conversion to finish
		while (!(LPC_ADC1->SEQ_GDAT[ADC_SEQA_IDX] & (1 << 31))) {
			uv_rtos_task_yield();
		}
#endif
		uint32_t raw = Chip_ADC_GetDataReg(LPC_ADC1, uv_ctz(channel));
		ret = ADC_DR_RESULT(raw);
	}
#endif

#endif

	return ret;
}



#if CONFIG_TARGET_LPC1549
#if (CONFIG_ADC_MODE == ADC_MODE_ASYNC)

void ADC0A_IRQHandler(void) {
	uint32_t pending = Chip_ADC_GetFlags(LPC_ADC0);
	Chip_ADC_ClearFlags(LPC_ADC0, pending);
	this->adc0_channels = 0;
	if (this->adc1_channels == 0) {
		// call callback function if both ADC's have finished their conversion
		CONFIG_ADC_CALLBACK ();
	}
}

void ADC1A_IRQHandler(void) {
	uint32_t pending = Chip_ADC_GetFlags(LPC_ADC1);
	Chip_ADC_ClearFlags(LPC_ADC1, pending);
	// call callback function and set adc1 channels to zero
	this->adc1_channels = 0;
	if (this->adc0_channels == 0) {
		// call callback function if both ADC's have finished their conversion
		CONFIG_ADC_CALLBACK ();
	}
}


void uv_adc_start(uv_adc_channels_e channels) {
	while (this->adc0_channels || this->adc1_channels) {
		uv_rtos_task_yield();
	}

	this->adc1_channels = (channels >> 12);
	this->adc0_channels = channels & 0b111111111111;

	// ADC0 conversions requested
	if (this->adc0_channels) {
		Chip_ADC_DisableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC0, ADC_SEQA_IDX,
				(this->adc0_channels) |
				ADC_SEQ_CTRL_HWTRIG_POLPOS | ADC_SEQ_CTRL_MODE_EOS);
		Chip_ADC_EnableSequencer(LPC_ADC0, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC0, ADC_SEQA_IDX);
	}
	// ADC1 conversions requested
	if (this->adc1_channels) {
		Chip_ADC_DisableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_SetupSequencer(LPC_ADC1, ADC_SEQA_IDX,
				(this->adc1_channels) |
				ADC_SEQ_CTRL_HWTRIG_POLPOS | ADC_SEQ_CTRL_MODE_EOS);
		Chip_ADC_EnableSequencer(LPC_ADC1, ADC_SEQA_IDX);
		Chip_ADC_StartSequencer(LPC_ADC1, ADC_SEQA_IDX);
	}
}

#endif
#endif



#if (CONFIG_ADC_MODE == ADC_MODE_SYNC)
int16_t uv_adc_read_average(uv_adc_channels_e channel, uint32_t conversion_count) {
	int32_t value = 0, i;
	for (i = 0; i < conversion_count; i++) {
		value += uv_adc_read(channel);
	}
	value /= conversion_count;
	return (int16_t) value;
}
#endif






#endif



