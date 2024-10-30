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



#include "uv_ltc6804.h"
#include "uv_rtos.h"
#include <string.h>


#if CONFIG_LTC6804

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
	spi_data_t write[9] = {}, read[sizeof(write) / sizeof(write[0])];
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
#endif
