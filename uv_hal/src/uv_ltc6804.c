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



#include "uv_ltc6804.h"
#include "uv_rtos.h"
#include <string.h>



static uint16_t crc_add_bits( uint16_t crc, uint16_t value, int bct )
{
    uint16_t mask = 1<<(bct-1);
    while( mask )
    {
       int bit = (value&mask) ? 1 : 0;
       if( crc & 0x4000 )
      bit = !bit;
       crc <<= 1;
       if( bit ) crc ^= 0x4599;
       mask>>=1;
    }

    return crc & 0x7FFF;
}

static uint16_t get_pec(uint8_t *data, uint16_t len) {
	uint16_t pec = 0x0010;

	for (uint32_t i = 0; i < len; i++) {
		pec = crc_add_bits(pec, data[i], 8);
	}

	// final value is shifted by one
	pec = pec << 1;

	return pec;
}

void uv_ltc6804_init(uv_ltc6804_st *this, spi_e spi, spi_slaves_e ssel) {
	this->spi = spi;
	this->ssel = ssel;

	// wake up the device with a dummy byte
	uint16_t write[9] = {}, read[sizeof(write) / sizeof(write[0])];
	write[0] = 0;
	uv_spi_write_sync(this->spi, this->ssel, write, 8, 1);
	uv_rtos_task_delay(1);

	// start adc conversion
	write[0] = 0x03;
	write[1] = 0x70;
	write[2] = 0xAF;
	write[3] = 0x42;
	uv_spi_write_sync(this->spi, this->ssel, write, 8, 4);

	uv_rtos_task_delay(10);

	// read cell voltages
	write[0] = 0x00;
	write[1] = 0x04;
	write[2] = 0x07;
	write[3] = 0xc2;
	memset(&write[4], 0, 5 * sizeof(write[0]));
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 9);

	printf("adc group 1:\n");
	for (uint8_t i = 0; i < 5; i++) {
		printf("   0x%x\n", read[4 + i]);
	}


}
