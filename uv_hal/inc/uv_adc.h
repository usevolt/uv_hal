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
#if CONFIG_ADC || CONFIG_ADC1 || CONFIG_ADC0

#define ADC_MODE_SYNC	1
#define ADC_MODE_ASYNC	2


#if CONFIG_TARGET_LPC1549
#define ADC_MAX_FREQ	50000000
#elif CONFIG_TARGET_LPC1785
#define ADC_MAX_FREQ	99999999
#endif

#if !defined(CONFIG_ADC_MODE)
#error "CONFIG_ADC_MODE should be defined as ADC_MODE_SYNC or ADC_MODE_ASYNC.\
 In ADC_MODE_SYNC the adc_read function waits for the conversion to finish and\
 returns the result. In ADC_MODE_ASYNC a separate adc_start function is needed\
 to start the conversion. After the conversion is done, result can be read\
 with adc_read."
#endif
#if ((CONFIG_ADC_MODE == ADC_MODE_ASYNC) && !defined(CONFIG_ADC_CALLBACK))
#error "CONFIG_ADC_CALLBACK should declare the adc callback function name in ASYNC mode.\
 Function returns void and takes no parameters (void)."
#endif
#if !CONFIG_ADC_CONVERSION_FREQ
#error "CONFIG_ADC_CONVERSION_FREQ should define the ADC conversion frequency in Hz."
#endif
#if ((CONFIG_ADC_CONVERSION_FREQ * 25) > ADC_MAX_FREQ)
#error "CONFIG_ADC_CONVERSION_FREQ exceeds the maximum frequency on this target MCU"
#endif


#if CONFIG_TARGET_LPC1785


enum {
	ADC_PULL_UP_ENABLED = (1 << 4),
	ADC_PULL_DOWN_ENABLED = (1 << 3),
};

/// @brief: Initializes an ADC pin to function as an analog channel
#define ADC_INIT(chn)	CAT(chn, _INIT)

#define ADC_0_INIT		do { LPC_IOCON->P0_23 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_23 |= (0b001); } while (0)

#define ADC_1_INIT		do { LPC_IOCON->P0_24 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_24 |= (0b001); } while (0)

#define ADC_2_INIT		do { LPC_IOCON->P0_25 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_25 |= (0b001); } while (0)

#define ADC_3_INIT		do { LPC_IOCON->P0_26 &= ~(0b111 | (1 << 7) | (1 << 16)); \
							LPC_IOCON->P0_26 |= (0b001); } while (0)

#define ADC_4_INIT		do { LPC_IOCON->P1_30 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P1_30 |= (0b011); } while (0)

#define ADC_5_INIT		do { LPC_IOCON->P1_31 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P1_31 |= (0b011); } while (0)

#define ADC_6_INIT		do { LPC_IOCON->P0_12 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_12 |= (0b011) } while (0)

#define ADC_7_INIT		do { LPC_IOCON->P0_13 &= ~(0b111 | (1 << 7)); \
							LPC_IOCON->P0_13 |= (0b011) } while (0)

/// @brief: Pull up & pull down configuration macros. These should be used
/// only to enable pull up & pull down resistors on AD pins, if necessary.
#define ADC_CONF(adc, confs) CAT(adc, _CONF(confs))

#define ADC_0_CONF(confs) do { LPC_IOCON->P0_23 |= confs; } while (0)
#define ADC_1_CONF(confs) do { LPC_IOCON->P0_24 |= confs; } while (0)
#define ADC_2_CONF(confs) do { LPC_IOCON->P0_25 |= confs; } while (0)
#define ADC_3_CONF(confs) do { LPC_IOCON->P0_26 |= confs; } while (0)
#define ADC_4_CONF(confs) do { LPC_IOCON->P0_30 |= confs; } while (0)
#define ADC_5_CONF(confs) do { LPC_IOCON->P0_31 |= confs; } while (0)
#define ADC_6_CONF(confs) do { LPC_IOCON->P0_12 |= confs; } while (0)
#define ADC_7_CONF(confs) do { LPC_IOCON->P0_13 |= confs; } while (0)

#endif


#if CONFIG_TARGET_LPC1549

#if !defined(CONFIG_ADC0)
#error "CONFIG_ADC0 should be defined as 0 or 1, depending if ADC0 is enabled."
#endif
#if !defined(CONFIG_ADC1)
#error "CONFIG_ADC1 should be defined as 0 or 1, depending if ADC1 is enabled."
#endif

#endif


/// @brief: Defines the ADC conversion max value ( == precision) for this hardware
enum {
#if CONFIG_TARGET_LPC11C14
	ADC_MAX_VALUE = 0x400
#elif CONFIG_TARGET_LPC1785
	ADC_MAX_VALUE = 0x1000
#elif CONFIG_TARGET_LPC1549
	ADC_MAX_VALUE = 0x1000,
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
	ADC_7 = (1 << 7)
#endif
#elif CONFIG_TARGET_LPC1549
#if CONFIG_ADC_CHANNEL0 || CONFIG_ADC_CHANNEL0_0
	ADC0_0 = (1 << 0),
#endif
#if CONFIG_ADC_CHANNEL1 || CONFIG_ADC_CHANNEL0_1
	ADC0_1 = (1 << 1),
#endif
#if CONFIG_ADC_CHANNEL2 || CONFIG_ADC_CHANNEL0_2
	ADC0_2 = (1 << 2),
#endif
#if CONFIG_ADC_CHANNEL3 || CONFIG_ADC_CHANNEL0_3
	ADC0_3 = (1 << 3),
#endif
#if CONFIG_ADC_CHANNEL4 || CONFIG_ADC_CHANNEL0_4
	ADC0_4 = (1 << 4),
#endif
#if CONFIG_ADC_CHANNEL5 || CONFIG_ADC_CHANNEL0_5
	ADC0_5 = (1 << 5),
#endif
#if CONFIG_ADC_CHANNEL6 || CONFIG_ADC_CHANNEL0_6
	ADC0_6 = (1 << 6),
#endif
#if CONFIG_ADC_CHANNEL7 || CONFIG_ADC_CHANNEL0_7
	ADC0_7 = (1 << 7),
#endif
#if CONFIG_ADC_CHANNEL8 || CONFIG_ADC_CHANNEL0_8
	ADC0_8 = (1 << 8),
#endif
#if CONFIG_ADC_CHANNEL9 || CONFIG_ADC_CHANNEL0_9
	ADC0_9 = (1 << 9),
#endif
#if CONFIG_ADC_CHANNEL10 || CONFIG_ADC_CHANNEL0_10
	ADC0_10 = (1 << 10),
#endif
#if CONFIG_ADC_CHANNEL11 || CONFIG_ADC_CHANNEL0_11
	ADC0_11 = (1 << 11),
#endif
#if CONFIG_ADC_CHANNEL12 || CONFIG_ADC_CHANNEL1_0
	ADC1_0 = (1 << 12),
#endif
#if CONFIG_ADC_CHANNEL13 || CONFIG_ADC_CHANNEL1_1
	ADC1_1 = (1 << 13),
#endif
#if CONFIG_ADC_CHANNEL14 || CONFIG_ADC_CHANNEL1_2
	ADC1_2 = (1 << 14),
#endif
#if CONFIG_ADC_CHANNEL15 || CONFIG_ADC_CHANNEL1_3
	ADC1_3 = (1 << 15),
#endif
#if CONFIG_ADC_CHANNEL16 || CONFIG_ADC_CHANNEL1_4
	ADC1_4 = (1 << 16),
#endif
#if CONFIG_ADC_CHANNEL17 || CONFIG_ADC_CHANNEL1_5
	ADC1_5 = (1 << 17),
#endif
#if CONFIG_ADC_CHANNEL18 || CONFIG_ADC_CHANNEL1_6
	ADC1_6 = (1 << 18),
#endif
#if CONFIG_ADC_CHANNEL19 || CONFIG_ADC_CHANNEL1_7
	ADC1_7 = (1 << 19),
#endif
#if CONFIG_ADC_CHANNEL20 || CONFIG_ADC_CHANNEL1_8
	ADC1_8 = (1 << 20),
#endif
#if CONFIG_ADC_CHANNEL21 || CONFIG_ADC_CHANNEL1_9
	ADC1_9 = (1 << 21),
#endif
#if CONFIG_ADC_CHANNEL22 || CONFIG_ADC_CHANNEL1_10
	ADC1_10 = (1 << 22),
#endif
#if CONFIG_ADC_CHANNEL23 || CONFIG_ADC_CHANNEL1_11
	ADC1_11 = (1 << 23)
#endif
#endif
} uv_adc_channels_e;


/// @brief: initialize adc converter.
uv_errors_e _uv_adc_init();



/// @brief: returns the channel'd adc channel value as 32-bit integer
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish. In ASYNC mode application should first call
/// *uv_adc_start* and wait for the conversion to finish.
/// For CONFIG_TARGET_LPC11C14 ADC has a 10 bit resolution -> return value is 0 - 1024.
/// For CONFIG_TARGET_LPC1785 ADC has a 12 bit resolution -> return value is 0 - 4096.
///
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
int16_t uv_adc_read(uv_adc_channels_e channel);


#if (CONFIG_ADC_MODE == ADC_MODE_ASYNC)
/// @brief: Trigger the start of ADC conversions. Multiple channels can be
/// selected for the conversion simultaneously if the target MCU supports it.
///
/// @param channels: OR'red channels for which the conversions will be triggered.
void uv_adc_start(uv_adc_channels_e channels);
#endif


#if (CONFIG_ADC_MODE == ADC_MODE_SYNC)
/// @brief: returns the channel'd adc channel value as 32-bit integer averaged by
/// 'conversion_count' times.
/// In burst operation this function executes fastly, otherwise the ADC conversion is triggered
/// and it takes 11 clock cycles to finish.
/// for CONFIG_TARGET_LPC11C14 adc has a 10 bit resolution -> return value is 0 - 1024
/// @return: Value from the adc, 0 ... ADC_MAX_VALUE
/// @param channel to be returned. Should be ADC_CHN_0, ADC_CHN_1, etc etc.
/// invalid channels return -1.
/// @param conversion_count: The amount of AD conversions to be done and averaged.
int16_t uv_adc_read_average(uv_adc_channels_e channel, uint32_t conversion_count);
#endif

#endif

#endif /* UW_ADC_H_ */
