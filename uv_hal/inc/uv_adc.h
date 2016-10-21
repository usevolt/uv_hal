/*
 *
 * uv_adc.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_ADC_H_
#define UW_ADC_H_

#include "uv_hal_config.h"

#include "uv_errors.h"
#include <stdbool.h>

#if CONFIG_ADC


/// @brief: Defines the ADC conversion max value ( == precision) for this hardware
enum {
#if CONFIG_TARGET_LPC11C14
	ADC_MAX_VALUE = 1023
#elif CONFIG_TARGET_LPC1785
	ADC_MAX_VALUE = 4095
#endif
};

/// @brief: Defines all ADC channels available on specific hardware
typedef enum {
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785
#if CONFIG_ADC_CHANNEL0
	ADC_CHN_0 = (1 << 0),
#endif
#if CONFIG_ADC_CHANNEL1
	ADC_CHN_1 = (1 << 1),
#endif
#if CONFIG_ADC_CHANNEL2
	ADC_CHN_2 = (1 << 2),
#endif
#if CONFIG_ADC_CHANNEL3
	ADC_CHN_3 = (1 << 3),
#endif
#if CONFIG_ADC_CHANNEL4
	ADC_CHN_4 = (1 << 4),
#endif
#if CONFIG_ADC_CHANNEL5
	ADC_CHN_5 = (1 << 5),
#endif
#if CONFIG_ADC_CHANNEL6
	ADC_CHN_6 = (1 << 6),
#endif
#if CONFIG_ADC_CHANNEL7
	ADC_CHN_7 = (1 << 7),
#endif
	ADC_CHN_COUNT = 8
#endif
} uv_adc_channels_e;


/// @brief: initialize adc converter.
/// uv_hal_config.h file should define used ADC channels as well as ADC mode
/// (CONTINUOUS / STANDARD).
uv_errors_e uv_adc_init();



/// @brief: returns the channel'd adc channel value as 32-bit integer
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// For CONFIG_TARGET_LPC11C14 ADC has a 10 bit resolution -> return value is 0 - 1024.
/// For CONFIG_TARGET_LPC1785 ADC has a 12 bit resolution -> return value is 0 - 4096.
///
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
int uv_adc_read(uv_adc_channels_e channel);


/// @brief: returns the channel'd adc channel value as 32-bit integer averaged by
/// 'conversion_count' times.
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// for CONFIG_TARGET_LPC11C14 adc has a 10 bit resolution -> return value is 0 - 1024
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
/// @param conversion_count: The amount of AD conversions to be done and averaged.
int uv_adc_read_average(uv_adc_channels_e channel, unsigned int conversion_count);


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
uv_errors_e uv_adc_start(uv_adc_channels_e channels);

/// @brief: Stops the ADC conversions from specific channels. Ongoing conversion will be
/// finished regardless.
///
/// @note: Useful specifically in the continuous mode.
///
/// @param channels: OR'red channels from which the conversions will be stopped
uv_errors_e uv_adc_stop(uv_adc_channels_e channels);


/// @brief: Adds a callback function which will be called when the ADC conversions are finished
uv_errors_e uv_adc_add_callback(
		void (*callback)(void *user_ptr, uv_adc_channels_e channel, unsigned int value));


***/

#endif

#endif /* UW_ADC_H_ */
