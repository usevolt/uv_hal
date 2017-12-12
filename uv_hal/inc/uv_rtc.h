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


/// @brief: Inits the real time clock.
void _uv_rtc_init();


/// @brief: Returns the current time to *dest*.
void uv_rtc_get_time(uv_time_st *dest);

/// @brief: Sets the current time
void uv_rtc_set_time(uv_time_st *src);



#endif

#endif /* UV_HAL_INC_UV_RTC_H_ */
