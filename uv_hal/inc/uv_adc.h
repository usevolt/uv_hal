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

#if CONFIG_TARGET_LPC1785

enum {
	ADC_PULL_UP_ENABLED = (1 << 4),
	ADC_PULL_DOWN_ENABLED = (1 << 3),
};

/// @brief: Initializes an ADC pin to function as an analog channel
#define ADC_INIT(chn)	CAT(chn, _INIT)

#define ADC_0_INIT		LPC_IOCON->P0_23 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_23 |= (0b001)

#define ADC_1_INIT		LPC_IOCON->P0_24 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_24 |= (0b001)

#define ADC_2_INIT		LPC_IOCON->P0_25 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_25 |= (0b001)

#define ADC_3_INIT		LPC_IOCON->P0_26 &= ~(0b111 | (1 << 7) | (1 << 16)); \
							LPC_IOCON->P0_26 |= (0b001)

#define ADC_4_INIT		LPC_IOCON->P1_30 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P1_30 |= (0b011)

#define ADC_5_INIT		LPC_IOCON->P1_31 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P1_31 |= (0b011)

#define ADC_6_INIT		LPC_IOCON->P0_12 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_12 |= (0b011)

#define ADC_7_INIT		LPC_IOCON->P0_13 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_13 |= (0b011)

/// @brief: Pull up & pull down configuration macros. These should be used
/// only to enable pull up & pull down resistors on AD pins, if necessary.
#define ADC_CONF(adc, confs) CAT(adc, _CONF(confs))

#define ADC_0_CONF(confs) LPC_IOCON->P0_23 |= confs
#define ADC_1_CONF(confs) LPC_IOCON->P0_24 |= confs
#define ADC_2_CONF(confs) LPC_IOCON->P0_25 |= confs
#define ADC_3_CONF(confs) LPC_IOCON->P0_26 |= confs
#define ADC_4_CONF(confs) LPC_IOCON->P0_30 |= confs
#define ADC_5_CONF(confs) LPC_IOCON->P0_31 |= confs
#define ADC_6_CONF(confs) LPC_IOCON->P0_12 |= confs
#define ADC_7_CONF(confs) LPC_IOCON->P0_13 |= confs

#endif




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
	ADC_0 = (1 << 0),
#endif
#if CONFIG_ADC_CHANNEL1
	ADC_1 = (1 << 1),
#endif
#if CONFIG_ADC_CHANNEL2
	ADC_2 = (1 << 2),
#endif
#if CONFIG_ADC_CHANNEL3
	ADC_3 = (1 << 3),
#endif
#if CONFIG_ADC_CHANNEL4
	ADC_4 = (1 << 4),
#endif
#if CONFIG_ADC_CHANNEL5
	ADC_5 = (1 << 5),
#endif
#if CONFIG_ADC_CHANNEL6
	ADC_6 = (1 << 6),
#endif
#if CONFIG_ADC_CHANNEL7
	ADC_7 = (1 << 7),
#endif
	ADC_COUNT = 8
#endif
} uv_adc_channels_e;


/// @brief: initialize adc converter.
uv_errors_e _uv_adc_init();



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


#endif /* UW_ADC_H_ */
