/*
 * uv_rtc.c
 *
 *  Created on: Oct 16, 2016
 *      Author: usevolt
 */


#include "uv_rtc.h"
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif


#if CONFIG_RTC




void _uv_rtc_init() {
	LPC_SC->PCONP |= (1 << 9);
	// disable the clock counters for initialization
	LPC_RTC->CCR = 1;

	// disable calibration
	LPC_RTC->CALIBRATION = 0;


}



void uv_rtc_get_time(uv_time_st *dest) {
	dest->hour = LPC_RTC->HOUR;
	dest->min = LPC_RTC->MIN;
	dest->sec = LPC_RTC->SEC;
	dest->year = LPC_RTC->YEAR;
	dest->month = LPC_RTC->MONTH;
	dest->day = LPC_RTC->DOM;
}


void uv_rtc_set_time(uv_time_st *src) {
	// disable RTC in order to enable access to the clock registers
	LPC_RTC->CCR = 0;
	LPC_RTC->SEC = src->sec;
	LPC_RTC->MIN = src->min;
	LPC_RTC->HOUR = src->hour;
	LPC_RTC->DOM = src->day;
	LPC_RTC->MONTH = src->month;
	LPC_RTC->YEAR = src->year;
	// enable rtc
	LPC_RTC->CCR = 1;
}





#endif
