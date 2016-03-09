/*
 * uw_adc_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#include <stdio.h>
#include "uw_adc.h"
#include "uw_uart.h"
#ifdef LPC11C14
#include "LPC11xx.h"
#elif defined(LPC1785)
#include "LPC177x_8x.h"
#endif


#ifdef LPC11C14
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
#elif defined(LPC1785)
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


uw_errors_e uw_init_adc(uw_adc_channels_e channels, int fosc, uw_adc_modes_e mode) {
#ifdef LPC11C14
	if ((channels & ADC_CHN_0)) {
		ADC_CHN_0_INIT;
	}
	if ((channels & ADC_CHN_1)) {
		ADC_CHN_1_INIT;
	}
	if ((channels & ADC_CHN_2)) {
		ADC_CHN_2_INIT;
	}
	if ((channels & ADC_CHN_3)) {
		ADC_CHN_3_INIT;
	}
	if ((channels & ADC_CHN_4)) {
		ADC_CHN_4_INIT;
	}
	if ((channels & ADC_CHN_5)) {
		ADC_CHN_5_INIT;
	}
	if ((channels & ADC_CHN_6)) {
		ADC_CHN_6_INIT;
	}
	if ((channels & ADC_CHN_7)) {
		ADC_CHN_7_INIT;
	}

	//enable clock to the adc
	LPC_SYSCON->SYSAHBCLKCTRL |= 1 << 13;
	//power on the adc
	LPC_SYSCON->PDRUNCFG &= ~(1 << 4);

	//set clock divider
	//divide system clock by desired conversion speed. Max speed is 4.5 MHz.
	//mask off irrelevant bits, then left-shift the result to CLKDIV place.
	LPC_ADC->CR |= ((fosc / 9000000) & 0xFF) << 8;

	switch (mode) {
	case ADC_CONTINUOUS_MODE:
		//disable interrupts as noted on LPC11C22 manual
		LPC_ADC->INTEN &= ~(0xFF);
		//set adc channels
		//mask of irrelevant bits from channels-variable
		LPC_ADC->CR |= channels & 0xff;
		//set adc to burst mode
		LPC_ADC->CR |= (1 << 16);
		break;
	case ADC_STANDARD_MODE:
		//set adc to software controlled mode
		LPC_ADC->CR &= ~(1 << 16);
		break;
	default:
		__uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_ADC);
	}
#elif defined(LPC1785)
	// put ADC off
	LPC_ADC->CR &= ~(1 << 21);

	if ((channels & ADC_CHN_0)) {
		ADC_CHN_0_INIT;
	}
	if ((channels & ADC_CHN_1)) {
		ADC_CHN_1_INIT;
	}
	if ((channels & ADC_CHN_2)) {
		ADC_CHN_2_INIT;
	}
	if ((channels & ADC_CHN_3)) {
		ADC_CHN_3_INIT;
	}
	if ((channels & ADC_CHN_4)) {
		ADC_CHN_4_INIT;
	}
	if ((channels & ADC_CHN_5)) {
		ADC_CHN_5_INIT;
	}
	if ((channels & ADC_CHN_6)) {
		ADC_CHN_6_INIT;
	}
	if ((channels & ADC_CHN_7)) {
		ADC_CHN_7_INIT;
	}
	//enable clock to the adc
	LPC_SC->PCONP |= (1 << 12);

	//set clock divider
	//divide system clock by desired conversion speed. Max speed is 12.4 MHz.
	//mask off irrelevant bits, then left-shift the result to CLKDIV place.
	LPC_ADC->CR |= (((fosc / 12000000) + 1) & 0xFF) << 8;

	switch (mode) {
	case ADC_CONTINUOUS_MODE:
		//disable interrupts as noted on LPC1785 manual
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
		break;
	case ADC_STANDARD_MODE:
		//set adc to software controlled mode
		LPC_ADC->CR &= ~(1 << 16);
		// dont start ADC now
		LPC_ADC->CR &= ~(0b111 << 24);
		// ADC ON
		LPC_ADC->CR|= (1 << 21);
		break;
	default:
		__uw_err_throw(ERR_UNSUPPORTED_PARAM3_VALUE | HAL_MODULE_ADC);
	}
#endif

	return ERR_NONE;
}




int uw_read_adc(uw_adc_channels_e channel) {
	//check if burst mode is on
	if ((LPC_ADC->CR >> 16) & 0x1) {
		//LPC11C14 has 8 channels, so channel has to be less than 8
		uint8_t i;
		for (i = 0; i < ADC_CHN_COUNT; i++) {
			if (channel & (1 << i)) {
#ifdef LPC11C14
				return (LPC_ADC->DR[i] >> 6) & 0x3FF;
#elif defined(LPC1785)
				return (LPC_ADC->DR[i] >> 4) & 0xFFF;
#endif
			}
		}
		// invalid channel
		return -1;
	}
	else {
		// if burst mode isn't on, trigger the AD conversion and return the value

		// check if the channel is valid
		if ((channel == 0) || ((channel & 0xFF) == 0)) {
			return -1;
		}
		// LPC11C22 has 8 channels, so channel has to be less than 8
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
#ifdef LPC11C14
		value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 6) & 0x3FF;
#elif defined(LPC1785)
		value = (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 4) & 0xFFF;
#endif
		return value;
	}
}

int uw_read_adc_average(uw_adc_channels_e channel, unsigned int conversion_count) {
	int value = 0, i;
	for (i = 0; i < conversion_count; i++) {
		value += uw_read_adc(channel);
	}
	value /= conversion_count;
	return value;
}



