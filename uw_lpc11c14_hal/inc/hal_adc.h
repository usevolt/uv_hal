/*
 * hal_adc.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef HAL_ADC_H_
#define HAL_ADC_H_


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

/// @brief: Describes the ADC modes.
/// in ADC_CONTINUOUS_MODE the ADC runs in the background all the time and
/// reading the adc values is fast since they can only be read from memory.
/// in ADC_STANDARD_MODE, the conversion is started when calling hal_read_adc.
/// The function waits for the conversion to finish and returns the value retrieved.
///
/// AD_CONTINUOUS_MODE is fast, but not precise since the output value is harder
/// to average. In ADC_STANDARD_MODE, conversion can be run multiple times and
/// average of the conversion can be taken.
typedef enum {
	ADC_CONTINUOUS_MODE,
	ADC_STANDARD_MODE
} hal_adc_modes_e;

/// @brief: initialize adc converter. AD converter runs continuously on the background.
/// @param channels OR'd channels that need to be turned on (ADC_CHN_0 | ADC_CHN_7, etc.)
/// Make sure no peripherals are activated on selected ad channel pins
/// @param fosc System oscillator frequency in Hz
/// @param mode: The mode in which the AD converter is used.
void hal_init_adc(unsigned int channels, int fosc, hal_adc_modes_e mode);


/// @brief: returns the channel'd adc channel value as 32-bit integer
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// for LPC11Cxx adc has a 10 bit resolution -> return value is 0 - 1024
/// @return: Value from the adc, 0 ... 1023
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return 0.
int hal_read_adc(unsigned int channel);


/// @brief: returns the channel'd adc channel value as 32-bit integer averaged by
/// 'conversion_count' times.
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// for LPC11Cxx adc has a 10 bit resolution -> return value is 0 - 1024
/// @return: Value from the adc, 0 ... 1023
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return 0.
/// @param conversion_count: The amount of AD conversions to be done and averaged.
int hal_read_adc_average(unsigned int channel, unsigned int conversion_count);

#endif /* HAL_ADC_H_ */
