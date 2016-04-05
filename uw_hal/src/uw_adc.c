/*
 * uw_adc_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#include "uw_adc.h"

#include <stdio.h>
#include "uw_uart.h"
#if CONFIG_TARGET_LPC11CXX
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC178X
#include "LPC177x_8x.h"
#endif


#if CONFIG_TARGET_LPC11CXX
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
#elif CONFIG_TARGET_LPC178X
#define ADC_CHN_0_INIT		LPC_IOCON->P0_23 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_23 |= (0b001)

#define ADC_CHN_1_INIT		LPC_IOCON->P0_24 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_24 |= (0b001)

#define ADC_CHN_2_INIT		LPC_IOCON->P0_25 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_25 |= (0b001)

#define ADC_CHN_3_INIT		LPC_IOCON->P0_26 &= ~(0b111 | (1 << 7) | (1 << 16)); \
							LPC_IOCON->P0_26 |= (0b001)

#define ADC_CHN_4_INIT		LPC_IOCON->P1_30 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P1_30 |= (0b011)

#define ADC_CHN_5_INIT		LPC_IOCON->P1_31 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P1_31 |= (0b011)

#define ADC_CHN_6_INIT		LPC_IOCON->P0_12 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_12 |= (0b011)

#define ADC_CHN_7_INIT		LPC_IOCON->P0_13 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_13 |= (0b011)
#endif


uw_errors_e uw_adc_init() {
#if CONFIG_TARGET_LPC11CXX

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

#if CONFIG_ADC_MODE_CONTINOUS
	//disable interrupts as noted on LPC11C22 manual
	LPC_ADC->INTEN &= ~(0xFF);
	//set adc channels
	//mask of irrelevant bits from channels-variable
	LPC_ADC->CR |= channels & 0xff;
	//set adc to burst mode
	LPC_ADC->CR |= (1 << 16);
#elif CONFIG_ADC_MODE_STANDARD
	//set adc to software controlled mode
	LPC_ADC->CR &= ~(1 << 16);
#endif

#elif CONFIG_TARGET_LPC178X
	// put ADC off
	LPC_ADC->CR &= ~(1 << 21);

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
	LPC_SC->PCONP |= (1 << 12);

	SystemCoreClockUpdate();

	//set clock divider
	//divide system clock by desired conversion speed. Max speed is 12.4 MHz.
	//mask off irrelevant bits, then left-shift the result to CLKDIV place.
	LPC_ADC->CR |= (((SystemCoreClock / 12000000) + 1) & 0xFF) << 8;

#if CONFIG_ADC_MODE_CONTINOUS
	//disable interrupts as noted on CONFIG_TARGET_LPC178X manual
	LPC_ADC->INTEN &= ~(0xFF);
	//set adc channels
	//mask of irrelevant bits from channels-variable
	LPC_ADC->CR &= ~(0xFF);
	LPC_ADC->CR |= channels & 0xff;
	//set adc to burst mode
	LPC_ADC->CR |= (1 << 16);
	// clear start bits
	LPC_ADC->CR &= ~(0b111 << 24);
	// set ADC ON
	LPC_ADC->CR |= (1 << 21);
#elif CONFIG_ADC_MODE_STANDARD
	//set adc to software controlled mode
	LPC_ADC->CR &= ~(1 << 16);
	// dont start ADC now
	LPC_ADC->CR &= ~(0b111 << 24);
	// ADC ON
	LPC_ADC->CR|= (1 << 21);
#endif

#endif

	return ERR_NONE;
}




int uw_adc_read(uw_adc_channels_e channel) {
#if CONFIG_ADC_MODE_CONTINOUS
	//check if burst mode is on
//	if ((LPC_ADC->CR >> 16) & 0x1) {
	//LPC11CXX has 8 channels, so channel has to be less than 8
	uint8_t i;
	for (i = 0; i < ADC_CHN_COUNT; i++) {
		if (channel & (1 << i)) {
#if CONFIG_TARGET_LPC11CXX
			return (LPC_ADC->DR[i] >> 6) & 0x3FF;
#elif CONFIG_TARGET_LPC178X
			return (LPC_ADC->DR[i] >> 4) & 0xFFF;
#endif
		}
	}
	// invalid channel
	return -1;
}
#elif CONFIG_ADC_MODE_STANDARD
	// if burst mode isn't on, trigger the AD conversion and return the value

	// check if the channel is valid
	// LPC11C22 has 8 channels, so channel has to be less than 8
	if ((channel & (channel - 1)) != 0) {
		__uw_log_error(ERR_UNSUPPORTED_PARAM1_VALUE | HAL_MODULE_ADC);
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
#if CONFIG_TARGET_LPC11CXX
	value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 6) & 0x3FF;
#elif CONFIG_TARGET_LPC178X
	value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 4) & 0xFFF;
#endif
	return value;
#endif
}

int uw_adc_read_average(uw_adc_channels_e channel, unsigned int conversion_count) {
	int value = 0, i;
	for (i = 0; i < conversion_count; i++) {
		value += uw_adc_read(channel);
	}
	value /= conversion_count;
	return value;
}


