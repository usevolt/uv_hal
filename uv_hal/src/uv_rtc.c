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
	uv_i2cm_readwrite(I2C0, addr, NULL, 0, read, sizeof(read));
	return (read[0] & (1 << 1));
}



void uv_rtc_get_time(uv_time_st *dest) {
	uint8_t addr = S35390A_CMD(S35390A_REALTIME_DATA1);
	uint8_t read[7] = { };
	uv_i2cm_readwrite(I2C0, addr, NULL, 0, read, sizeof(read));

	for (uint8_t i = 0; i < 7; i++) {
		printf("0x%x ", read[i]);
	}
	printf("0x%x\n", bitswap(0xF));


	dest->year = 2000 + bitswap(read[0] << 4) * 10 + bitswap(read[0] & 0xF0);
	dest->month = bitswap((read[1] << 4)) * 10 + bitswap(read[1] & 0xF0);
	dest->day = bitswap((read[2] << 4)) * 10 + bitswap(read[2] & 0xF0);
	dest->hour = bitswap(((read[4] & 0xC) << 4)) * 10 + bitswap(read[4] & 0xF0);
	dest->min = bitswap((read[5] << 4)) * 10 + bitswap(read[5] & 0xF0);
	dest->sec = bitswap((read[6] << 4)) * 10 + bitswap(read[6] & 0xF0);

}


void uv_rtc_set_time(uv_time_st *src) {

	uint8_t write[7] = { };

	// start by setting the hour mode to 24h
	uint8_t addr = S35390A_CMD(S35390A_STATUS_REG1);
	write[0] = 0x40;
	uv_i2cm_readwrite(I2C0, addr, write, 1, NULL, 0);

	addr = S35390A_CMD(S35390A_REALTIME_DATA1);
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

	uv_i2cm_readwrite(I2C0, addr, write, sizeof(write), NULL, 0);
}





#endif
