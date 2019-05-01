/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UV_HAL_INC_UV_RTC_H_
#define UV_HAL_INC_UV_RTC_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"


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
typedef struct {
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
void uv_rtc_get_time(uv_time_st *dest);

/// @brief: Sets the current time
/// With S35390A this communicates with I2C bus, thus the execution takes a long time.
void uv_rtc_set_time(uv_time_st *src);



#endif

#endif /* UV_HAL_INC_UV_RTC_H_ */
