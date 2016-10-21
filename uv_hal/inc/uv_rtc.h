/*
 * uv_rtc.h
 *
 *  Created on: Oct 16, 2016
 *      Author: usevolt
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
