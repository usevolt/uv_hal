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


#if CONFIG_RTC


#define S35390A_DEVICE_CODE				(0b0110)
#define S35390A_CMD(cmd)				((S35390A_DEVICE_CODE << 3) | (cmd))
#define S35390A_STATUS_REG1				0b000
#define S35390A_STATUS_REG2				0b001
#define S35390A_REALTIME_DATA1			0b010
#define S35390A_REALTIME_DATA2			0b011
#define S35390A_INT1_REG				0b100
#define S35390A_INT2_REG				0b101
#define S35390A_CLOCK_CORR_REG			0b110
#define S35390A_FREE_REG				0b111



uint8_t bitswap(uint8_t b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}

bool uv_rtc_get_low_power_flag(void) {
	uint8_t addr = S35390A_CMD(S35390A_STATUS_REG1);
	uint8_t read[1] = { };
//	uv_i2cm_read(I2C0, addr, NULL, 0, read, sizeof(read));
	return (read[0] & (1 << 1));
}



void uv_rtc_get_time(uv_time_st *dest) {
	uint8_t addr = S35390A_CMD(S35390A_REALTIME_DATA1);
	uint8_t read[7] = { };
//	uv_i2cm_readwrite(I2C0, addr, NULL, 0, read, sizeof(read));

	dest->year = 2000 + bitswap(read[0] << 4) * 10 + bitswap(read[0] & 0xF0);
	dest->month = bitswap((read[1] << 4)) * 10 + bitswap(read[1] & 0xF0);
	dest->day = bitswap((read[2] << 4)) * 10 + bitswap(read[2] & 0xF0);
	dest->hour = bitswap(((read[4] & 0xC) << 4)) * 10 + bitswap(read[4] & 0xF0);
	dest->min = bitswap((read[5] << 4)) * 10 + bitswap(read[5] & 0xF0);
	dest->sec = bitswap((read[6] << 4)) * 10 + bitswap(read[6] & 0xF0);

	uint16_t buildyear = strtol(&__DATE__[7], NULL, 0);

	if (dest->year < buildyear) {
		// past date given, update the time with build date
		dest->year = strtol(&__DATE__[7], NULL, 0);
		char *months[] = {
				"Jan",
				"Feb",
				"Mar",
				"Apr",
				"May",
				"Jun",
				"Jul",
				"Aug",
				"Sep",
				"Oct",
				"Nov",
				"Dec"
		};
		for (uint8_t i = 0; i < 12; i++) {
			if (strstr(__DATE__, months[i])) {
				dest->month = i + 1;
				break;
			}
		}
		dest->day = strtol(&__DATE__[3], NULL, 0);
		dest->hour = strtol(__TIME__, NULL, 0);
		dest->min = strtol(&__TIME__[3], NULL, 0);
		dest->sec = strtol(&__TIME__[6], NULL, 0);

		uv_rtc_set_time(dest);
	}

}


void uv_rtc_set_time(uv_time_st *src) {

	uint8_t write[7] = { };

	// start by setting the hour mode to 24h
	uint8_t addr = S35390A_CMD(S35390A_STATUS_REG1);
	write[0] = 0x40;
//	uv_i2cm_readwrite(I2C0, addr, write, 1, NULL, 0);

	addr = S35390A_CMD(S35390A_REALTIME_DATA1);
	// get the base year from the build date
	int16_t year = src->year - 2000;
	if (year < 0) {
		year = 0;
	}
	write[0] = (bitswap(year / 10) >> 4) + bitswap(year % 10);
	write[1] = (bitswap(src->month / 10) >> 4) + bitswap(src->month % 10);
	write[2] = (bitswap(src->day / 10) >> 4) + bitswap(src->day % 10);
	write[3] = 0;
	write[4] = (bitswap(src->hour / 10) >> 4) + bitswap(src->hour % 10);
	write[5] = (bitswap(src->min / 10) >> 4) + bitswap(src->min % 10);
	write[6] = (bitswap(src->sec / 10) >> 4) + bitswap(src->sec % 10);

//	uv_i2cm_readwrite(I2C0, addr, write, sizeof(write), NULL, 0);
}





#endif
