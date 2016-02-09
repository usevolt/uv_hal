/*
 * uw_adc_controller.c
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#include "hal_adc_controller.h"
#include "hal_uart.h"
#include <stdio.h>

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

static volatile int conversion_count;


void hal_init_adc(unsigned int channels, int fosc) {

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
	//todo: increase conversion speed to 4 MHz
	LPC_ADC->CR |= (fosc / 9000000) << 8;

	//disable interrupts as noted on LPC11C22 manual
	LPC_ADC->INTEN &= ~(1 << 8);
	//set adc channels
	//mask of irrelevant bits from channels-variable
	LPC_ADC->CR |= channels & 0xff;
	//set adc to burst mode
	LPC_ADC->CR |= (1 << 16);

}

void hal_init_adc_std(unsigned int channels, int fosc, int average_count) {
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
	// @note: LPC11C22 datasheet claims that 4.5 MHz is
	LPC_ADC->CR |= (fosc / 900000) << 8;

//	//disable interrupts as noted on LPC11C22 manual
//	LPC_ADC->INTEN &= ~(1 << 8);
	//set adc to software controlled mode
	LPC_ADC->CR &= ~(1 << 16);

	// set the requested conversion count
	conversion_count = average_count;
}


int hal_read_adc(unsigned int channel) {

	//check if burst mode is on
	if ((LPC_ADC->CR >> 16) & 0x1) {
		//LPC11C22 has 8 channels, so channel has to be less than 8
		switch (channel) {
		case ADC_CHN_0:
			return (LPC_ADC->DR[0] >> 6) & 0x3FF;
		case ADC_CHN_1:
			return (LPC_ADC->DR[1] >> 6) & 0x3FF;
		case ADC_CHN_2:
			return (LPC_ADC->DR[2] >> 6) & 0x3FF;
		case ADC_CHN_3:
			return (LPC_ADC->DR[3] >> 6) & 0x3FF;
		case ADC_CHN_4:
			return (LPC_ADC->DR[4] >> 6) & 0x3FF;
		case ADC_CHN_5:
			return (LPC_ADC->DR[5] >> 6) & 0x3FF;
		case ADC_CHN_6:
			return (LPC_ADC->DR[6] >> 6) & 0x3FF;
		case ADC_CHN_7:
			return (LPC_ADC->DR[7] >> 6) & 0x3FF;
		default:
			return 0;	//set adc channels
		}

		//mask of irrelevant bits from channels-variable
		LPC_ADC->CR |= channel & 0xff;

	}
	else {
		// if burst mode isn't on, trigger the AD conversion
		// 'conversion_count' times and return the average value
		// LPC11C22 has 8 channels, so channel has to be less than 8
		int i, value = 0;
		switch (channel) {
		case ADC_CHN_0:
		case ADC_CHN_1:
		case ADC_CHN_2:
		case ADC_CHN_3:
		case ADC_CHN_4:
		case ADC_CHN_5:
		case ADC_CHN_6:
		case ADC_CHN_7:
			break;
		default:
			return 0;
		}
		for (i = 0; i < conversion_count; i++) {
			// make sure that only one channel is selected
			//set the channel
			LPC_ADC->CR &= ~(ADC_CHN_ALL);
			LPC_ADC->CR |= channel;
			// start the conversion
			LPC_ADC->CR &= ~(0b111 << 24);
			LPC_ADC->CR |= (1 << 24);
			//wait until the conversion is finished
			while (!(LPC_ADC->STAT & (1 << 16)));
			//read the acquired value
			value += (LPC_ADC->DR[(LPC_ADC->GDR >> 24) & 0b111] >> 6) & 0x3FF;

		}
		value /= conversion_count;
		return value;
	}
	return 0;
}

