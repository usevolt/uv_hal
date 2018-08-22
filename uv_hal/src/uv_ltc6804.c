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
}
