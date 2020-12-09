/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef UW_ADC_H_
#define UW_ADC_H_

#include "uv_hal_config.h"

#include "uv_errors.h"
#include <stdbool.h>
#include <stdint.h>
#include <uv_gpio.h>
#if CONFIG_ADC || CONFIG_ADC1 || CONFIG_ADC0


#define ADC_MAX_FREQ	50000000

#if !CONFIG_ADC_CONVERSION_FREQ
#error "CONFIG_ADC_CONVERSION_FREQ should define the ADC conversion frequency in Hz."
#endif
#if ((CONFIG_ADC_CONVERSION_FREQ * 25) > ADC_MAX_FREQ)
#error "CONFIG_ADC_CONVERSION_FREQ exceeds the maximum frequency on this target MCU"
#endif



#if !defined(CONFIG_ADC0)
#error "CONFIG_ADC0 should be defined as 0 or 1, depending if ADC0 is enabled."
#endif
#if !defined(CONFIG_ADC1)
#error "CONFIG_ADC1 should be defined as 0 or 1, depending if ADC1 is enabled."
#endif


/// @brief: Defines the ADC conversion max value ( == precision) for this hardware
enum {
	ADC_MAX_VALUE = 0x1000
};

/// @brief: Defines all ADC channels available on specific hardware.
///
/// @note: These enums have to be in ascending order, starting from 1. 0 is read as NULL value.
/// The last *ADC_COUNT* has to define the number of ADC channels on the system plus 1.
typedef enum {
	ADC0_0 = 1,
	ADC0_1,
	ADC0_2,
	ADC0_3,
	ADC0_4,
	ADC0_5,
	ADC0_6,
	ADC0_7,
	ADC0_8,
	ADC0_9,
	ADC0_10,
	ADC0_11,
	ADC1_0,
	ADC1_1,
	ADC1_2,
	ADC1_3,
	ADC1_4,
	ADC1_5,
	ADC1_6,
	ADC1_7,
	ADC1_8,
	ADC1_9,
	ADC1_10,
	ADC1_11,
	ADC_COUNT
} uv_adc_channels_e;


/// @brief: initialize adc converter.
uv_errors_e _uv_adc_init();


/// @brief: returns the channel'd adc channel value as 32-bit integer
///
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
int16_t uv_adc_read(uv_adc_channels_e channel);


/// @brief: Makes the analog conversion *conversion_count* times and
/// returns the average value.
///
/// @param conversion_count: The amount of AD conversions to be done and averaged.
int16_t uv_adc_read_average(uv_adc_channels_e channel, uint32_t conversion_count);


/// @brief: Enabled and initializes the given AIN pin. The GPIO is set in analog
/// mode and all digital functions are disabled.
void uv_adc_enable_ain(uv_adc_channels_e channel);



/// @brief: Returns the GPIO pin for the given adc channel
uv_gpios_e uv_adc_get_gpio_pin(uv_adc_channels_e channel);



/// @brief: Disabled the given AIN pin. The GPIO pin is set in digital mode
/// without any additional functionality.
void uv_adc_disable_ain(uv_adc_channels_e channel);

#endif

#endif /* UW_ADC_H_ */
