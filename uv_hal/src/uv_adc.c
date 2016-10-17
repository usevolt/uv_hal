/*
 * uv_adc_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#include "uv_adc.h"

#include <stdio.h>
#include "uv_uart.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif

#if CONFIG_ADC

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

#endif

	return uv_err(ERR_NONE);
}




int uv_adc_read(uv_adc_channels_e channel) {
	// if burst mode isn't on, trigger the AD conversion and return the value

	// check if the channel is valid
	// LPC11C22 has 8 channels, so channel has to be less than 8
	if ((channel & (channel - 1)) != 0) {
		__uv_log_error(ERR_UNSUPPORTED_PARAM1_VALUE | HAL_MODULE_ADC);
		return -1;
	}
	int value = 0;
	// make sure that only one channel is selected
	//set the channel
	LPC_ADC->CR &= ~(0xFF);
	LPC_ADC->CR |= channel & 0xFF;
	// start the conversion
	LPC_ADC->CR &= ~(0b111 << 24);
	LPC_ADC->CR |= (1 << 24);
	//wait until the conversion is finished
	while (!(LPC_ADC->STAT & (1 << 16)));
	//read the acquired value
#if CONFIG_TARGET_LPC11C14
	value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 6) & 0x3FF;
#elif CONFIG_TARGET_LPC1785
	value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 4) & 0xFFF;
#endif
	return value;
}

int uv_adc_read_average(uv_adc_channels_e channel, unsigned int conversion_count) {
	int value = 0, i;
	for (i = 0; i < conversion_count; i++) {
		value += uv_adc_read(channel);
	}
	value /= conversion_count;
	return value;
}

#endif
