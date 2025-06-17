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

#ifndef UV_HAL_INC_UV_RTC_H_
#define UV_HAL_INC_UV_RTC_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_i2c.h"

#if CONFIG_RTC

/// @file: Defines a Real time clock module. Since LPC1549 errata sheet
/// prohibits the use of internal RTC on LPC1549, an I2C RTC component S-35390A
/// is used instead.


#if !CONFIG_I2C
#error "RTC module requires I2C to be enabled with CONFIG_I2C."
#endif


/// @brief: Time structure which is used to represent time.
/// newlib time_t and struct tm are more complicated and
/// they would add ~10 kt to the program size
typedef struct __attribute__((packed)) {
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
} uv_time_st;


/// @brief: Returns true if the RTC module detects a low power warning
bool uv_rtc_get_low_power_flag(void);

/// @brief: Returns the current time to *dest*.
/// With S35390A this communicates with I2C bus, thus the execution takes a long time.
void uv_rtc_get_time(uv_time_st *dest, i2c_e i2c);

/// @brief: Sets the current time
/// With S35390A this communicates with I2C bus, thus the execution takes a long time.
void uv_rtc_set_time(uv_time_st *src, i2c_e i2c);



#endif

#endif /* UV_HAL_INC_UV_RTC_H_ */
