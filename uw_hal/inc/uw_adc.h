/*
 * uw_adc.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_ADC_H_
#define UW_ADC_H_

#include "uw_errors.h"
#include <stdbool.h>

/// @brief: Defines the ADC conversion max value ( == precision) for this hardware
enum {
#ifdef LPC11C14
	ADC_MAX_VALUE = 1024
#elif defined(LPC1785)
	ADC_MAX_VALUE = 4096
#endif
};

/// @brief: Defines all ADC channels available on specific hardware
typedef enum {
#if defined(LPC11C14) || defined(LPC1785)
	ADC_CHN_0 = (1 << 0),
	ADC_CHN_1 = (1 << 1),
	ADC_CHN_2 = (1 << 2),
	ADC_CHN_3 = (1 << 3),
	ADC_CHN_4 = (1 << 4),
	ADC_CHN_5 = (1 << 5),
	ADC_CHN_6 = (1 << 6),
	ADC_CHN_7 = (1 << 7),
	ADC_CHN_COUNT = 8
#endif
} uw_adc_channels_e;

/// @brief: Describes the ADC modes.
/// in ADC_CONTINUOUS_MODE the ADC runs in the background all the time and
/// reading the adc values is fast since they can only be read from memory.
/// in ADC_STANDARD_MODE, the conversion is started when calling uw_read_adc.
/// The function waits for the conversion to finish and returns the value retrieved.
///
/// AD_CONTINUOUS_MODE is fast, but not precise since the output value is harder
/// to average. In ADC_STANDARD_MODE, conversion can be run multiple times and
/// average of the conversion can be taken.
typedef enum {
	ADC_CONTINUOUS_MODE,
	ADC_STANDARD_MODE
} uw_adc_modes_e;

/// @brief: initialize adc converter. AD converter runs continuously on the background.
/// @param channels OR'd channels that need to be turned on (ADC_CHN_0 | ADC_CHN_7, etc.)
/// Make sure no peripherals are activated on selected ad channel pins
/// @param fosc System oscillator frequency in Hz
/// @param mode: The mode in which the AD converter is used.
uw_errors_e uw_adc_init(uw_adc_channels_e channels, int fosc, uw_adc_modes_e mode);


/// @brief: returns the channel'd adc channel value as 32-bit integer
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// For LPC11Cxx ADC has a 10 bit resolution -> return value is 0 - 1024.
/// For LPC1785 ADC has a 12 bit resolution -> return value is 0 - 4096.
///
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
int uw_adc_read(uw_adc_channels_e channel);


/// @brief: returns the channel'd adc channel value as 32-bit integer averaged by
/// 'conversion_count' times.
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// for LPC11Cxx adc has a 10 bit resolution -> return value is 0 - 1024
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
/// @param conversion_count: The amount of AD conversions to be done and averaged.
int uw_adc_read_average(uw_adc_channels_e channel, unsigned int conversion_count);


/*** YET UNIMPLEMENTED METHODS

/// @brief: Starts the ADC conversion but doesn't wait for it to finish.
/// Multiple channels can be started at the same time.
///
/// @note: Works in both continuous and standard mode. In continuous mode,
/// This triggers the start of the continuous conversions.
/// In standard mode, only one conversion per channel is made.
/// Callback function will be called when the conversion is done and the result can be read.
///
/// @param channels: OR'red channels which will be converted.
uw_errors_e uw_adc_start(uw_adc_channels_e channels);

/// @brief: Stops the ADC conversions from specific channels. Ongoing conversion will be
/// finished regardless.
///
/// @note: Useful specifically in the continuous mode.
///
/// @param channels: OR'red channels from which the conversions will be stopped
uw_errors_e uw_adc_stop(uw_adc_channels_e channels);


/// @brief: Adds a callback function which will be called when the ADC conversions are finished
uw_errors_e uw_adc_add_callback(
		void (*callback)(void *user_ptr, uw_adc_channels_e channel, unsigned int value));


***/

#endif /* UW_ADC_H_ */
