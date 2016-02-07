/*
 * uw_adc_controller.h
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#ifndef HAL_ADC_CONTROLLER_H_
#define HAL_ADC_CONTROLLER_H_


#include "LPC11xx.h"
#include <stdbool.h>


#define ADC_CHN_0	(1 << 0)
#define ADC_CHN_1 	(1 << 1)
#define ADC_CHN_2	(1 << 2)
#define ADC_CHN_3	(1 << 3)
#define ADC_CHN_4	(1 << 4)
#define ADC_CHN_5	(1 << 5)
#define ADC_CHN_6	(1 << 6)
#define ADC_CHN_7	(1 << 7)
#define ADC_CHN_ALL (0xFF)

/// @brief: initialize adc converter. AD converter runs continuously on the background.
/// @param channels OR'd channels that need to be turned on (ADC_CHN_0 | ADC_CHN_7, etc.)
/// Make sure no peripherals are activated on selected ad channel pins
/// @param fosc System oscillator frequency in Hz
void hal_init_adc(unsigned int channels, int fosc);

/// @brief: initialize adc converter "the standard way". The AD conversion is done only
/// when called, which means that reading the channels is slower but it allows averaging the data.
/// @param channels OR'd channels that need to be turned on (ADC_CHN_0 | ADC_CHN_7, etc.)
/// Make sure no peripherals are activated on selected ad channel pins
/// @param fosc System oscillator frequency in Hz
/// @param average_count The count how many times one AD conversion is done in one call.
/// Afterwards the average value of all conversions is returned.
void hal_init_adc_std(unsigned int channels, int fosc, int average_count);

/// @brief: returns the channel'd adc channel value as 32-bit integer
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// for LPC11C22 adc has a 10 bit resolution -> return value is 0 - 1024
/// @return: Value from the adc, 0 ... 1023
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return 0.
int hal_read_adc(unsigned int channel);

#endif /* HAL_ADC_CONTROLLER_H_ */
