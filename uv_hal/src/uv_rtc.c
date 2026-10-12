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


#include "uv_rtc.h"
#include <uv_i2c.h>
#include <uv_rtos.h>
#include <string.h>


#if CONFIG_RTC


#define S35390A_DEVICE_CODE				(0b0110)
#define S35390A_CMD(cmd, rw)			((((S35390A_DEVICE_CODE << 3) | (cmd)) << 1) | rw)
#define S35390A_STATUS_REG1				0b000
#define S35390A_STATUS_REG2				0b001
#define S35390A_REALTIME_DATA1			0b010
#define S35390A_REALTIME_DATA2			0b011
#define S35390A_INT1_REG				0b100
#define S35390A_INT2_REG				0b101
#define S35390A_CLOCK_CORR_REG			0b110
#define S35390A_FREE_REG				0b111



// How long the RTC is left alone after a failed transfer. Every transfer to a
// missing RTC costs an I2C timeout in the calling task, so while it is not
// answering it is only retried this often. In between the time is kept by
// the software clock.
#if !defined(CONFIG_RTC_RETRY_MS)
#define CONFIG_RTC_RETRY_MS				10000
#endif


// Software clock. Holds the last time read from or written to the RTC and the
// tick count it was taken at, and carries the time on from the RTOS tick when
// the RTC does not answer.
static struct {
	uv_time_st time;
	uint32_t tick;
	uint32_t fail_tick;
	bool valid;
	bool failed;
} rtc = {};


uint8_t bitswap(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


// Returns true if the RTC can be accessed: it answered last time, or
// CONFIG_RTC_RETRY_MS has passed since it last failed.
static bool rtc_available(void) {
	bool ret = true;
	if (rtc.failed) {
		uint32_t retry = (CONFIG_RTC_RETRY_MS * uv_rtos_get_tick_rate_hz()) / 1000U;
		ret = ((uv_rtos_get_tick_count() - rtc.fail_tick) >= retry);
	}
	return ret;
}


// Records the result of an RTC transfer
static void rtc_result(uv_errors_e err) {
	if (err == ERR_NONE) {
		rtc.failed = false;
	}
	else {
		rtc.failed = true;
		rtc.fail_tick = uv_rtos_get_tick_count();
	}
}


// Sets the software clock to *time*, taken at the current tick
static void soft_set(const uv_time_st *time) {
	rtc.time = *time;
	rtc.tick = uv_rtos_get_tick_count();
	rtc.valid = true;
}


static uint8_t days_in_month(uint16_t year, uint8_t month) {
	static const uint8_t days[12] = {
			31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint8_t ret = 31;
	if ((month >= 1U) && (month <= 12U)) {
		ret = days[month - 1U];
		if ((month == 2U) &&
				((((year % 4U) == 0U) && ((year % 100U) != 0U)) ||
				((year % 400U) == 0U))) {
			ret = 29;
		}
	}
	return ret;
}


// Adds *secs* seconds to *time*, carrying over to the date
static void add_seconds(uv_time_st *time, uint32_t secs) {
	uint32_t total = time->sec + secs;
	time->sec = total % 60U;
	total = time->min + (total / 60U);
	time->min = total % 60U;
	total = time->hour + (total / 60U);
	time->hour = total % 24U;
	uint32_t days = total / 24U;
	while (days > 0U) {
		days--;
		time->day++;
		if (time->day > days_in_month(time->year, time->month)) {
			time->day = 1;
			time->month++;
			if (time->month > 12U) {
				time->month = 1;
				time->year++;
			}
		}
	}
}


// Writes the firmware build date and time to *dest*
static void build_time(uv_time_st *dest) {
	static const char *months[] = {
			"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	dest->year = strtol(&__DATE__[7], NULL, 0);
	dest->month = 1;
	for (uint8_t i = 0; i < 12; i++) {
		if (strstr(__DATE__, months[i])) {
			dest->month = i + 1;
			break;
		}
	}
	dest->day = strtol(&__DATE__[3], NULL, 10);
	dest->hour = strtol(__TIME__, NULL, 10);
	dest->min = strtol(&__TIME__[3], NULL, 10);
	dest->sec = strtol(&__TIME__[6], NULL, 10);
}


// Returns the software clock's time to *dest*. Starts from the build date
// if the time has never been known.
static void soft_get(uv_time_st *dest) {
	if (!rtc.valid) {
		uv_time_st t;
		build_time(&t);
		soft_set(&t);
	}
	// move the base on by the whole seconds passed, so that the remainder
	// carries over and the tick difference never wraps
	uint32_t rate = uv_rtos_get_tick_rate_hz();
	uint32_t secs = (uv_rtos_get_tick_count() - rtc.tick) / rate;
	add_seconds(&rtc.time, secs);
	rtc.tick += secs * rate;
	*dest = rtc.time;
}


bool uv_rtc_get_low_power_flag(void) {
	bool ret = false;
	if (rtc_available()) {
		uint8_t read[2] = { S35390A_CMD(S35390A_STATUS_REG1, I2C_READ) };
		uv_errors_e err = uv_i2cm_read(I2C0, read, 1, read, 1);
		rtc_result(err);
		ret = (err == ERR_NONE) && ((read[1] & (1 << 1)) != 0U);
	}
	return ret;
}



void uv_rtc_get_time(uv_time_st *dest, i2c_e i2c) {
	uv_errors_e err = ERR_NOT_RESPONDING;
	if (rtc_available()) {
		uint8_t read[8] = { S35390A_CMD(S35390A_REALTIME_DATA1, I2C_READ) };
		err = uv_i2cm_read(i2c, NULL, 0, read, sizeof(read));
		rtc_result(err);
		if (err == ERR_NONE) {
			dest->year = 2000 + bitswap(read[1] << 4) * 10 + bitswap(read[1] & 0xF0);
			dest->month = bitswap((read[2] << 4)) * 10 + bitswap(read[2] & 0xF0);
			dest->day = bitswap((read[3] << 4)) * 10 + bitswap(read[3] & 0xF0);
			dest->hour = bitswap(((read[5] & 0xC) << 4)) * 10 + bitswap(read[5] & 0xF0);
			dest->min = bitswap((read[6] << 4)) * 10 + bitswap(read[6] & 0xF0);
			dest->sec = bitswap((read[7] << 4)) * 10 + bitswap(read[7] & 0xF0);

			uint16_t buildyear = strtol(&__DATE__[7], NULL, 0);
			if (dest->year < buildyear) {
				// past date given, update the time with build date
				build_time(dest);
				uv_rtc_set_time(dest, i2c);
			}
			else {
				soft_set(dest);
			}
		}
	}
	if (err != ERR_NONE) {
		// RTC not answering: carry on with the software clock
		soft_get(dest);
	}
}


void uv_rtc_set_time(uv_time_st *src, i2c_e i2c) {
	soft_set(src);

	if (rtc_available()) {
		uint8_t write[8] = { S35390A_CMD(S35390A_STATUS_REG1, I2C_WRITE) };

		write[1] = 0x40;
		uv_errors_e err = uv_i2cm_write(i2c, write, 2);

		if (err == ERR_NONE) {
			write[0] = S35390A_CMD(S35390A_REALTIME_DATA1, I2C_WRITE);
			// get the base year from the build date
			int16_t year = src->year - 2000;
			if (year < 0) {
				year = 0;
			}
			write[1] = (bitswap(year / 10) >> 4) + bitswap(year % 10);
			write[2] = (bitswap(src->month / 10) >> 4) + bitswap(src->month % 10);
			write[3] = (bitswap(src->day / 10) >> 4) + bitswap(src->day % 10);
			write[4] = 0;
			write[5] = (bitswap(src->hour / 10) >> 4) + bitswap(src->hour % 10);
			write[6] = (bitswap(src->min / 10) >> 4) + bitswap(src->min % 10);
			write[7] = (bitswap(src->sec / 10) >> 4) + bitswap(src->sec % 10);

			err = uv_i2cm_write(i2c, write, sizeof(write));
		}
		rtc_result(err);
	}
}





#endif
