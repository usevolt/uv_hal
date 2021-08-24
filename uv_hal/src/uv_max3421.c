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



#include "uv_max3421.h"

#if CONFIG_MAX3421

#include "uv_rtos.h"
#include "uv_terminal.h"


typedef enum {
	REG_PERIPH_EP0FIFO = 0,
	REG_PERIPH_EP1OUTFIFO = 1,
	REG_PERIPH_EP2INFIFO = 2,
	REG_PERIPH_EP3INFIFO = 3,
	REG_PERIPH_SUDFIFO = 4,
	REG_PERIPH_EP0BC = 5,
	REG_PERIPH_EP1OUTBC = 6,
	REG_PERIPH_EP2INBC = 7,
	REG_PERIPH_EP3INBC = 8,
	REG_PERIPH_EPSTALLS = 9,
	REG_PERIPH_CLRTOGS = 10,
	REG_PERIPH_EPIRQ = 11,
	REG_PERIPH_EPIEN = 12,
	REG_PERIPH_USBIRQ = 13,
	REG_PERIPH_USBIEN = 14,
	REG_PERIPH_USBCTL = 15,
	REG_PERIPH_CPUCTL = 16,
	REG_PERIPH_PINCTL = 17,
	REG_PERIPH_REVISION = 18,
	REG_PERIPH_FNADDR = 19,
	REG_PERIPH_IOPINS1 = 20,
	REG_PERIPH_IOPINS2 = 21,
	REG_PERIPH_GPINIRQ = 22,
	REG_PERIPH_GPINIEN = 23,
	REG_PERIPH_GPINPOL = 24,
	REG_PERIPH_MODE = 27,

	REG_HOST_RCVFIFO = 1,
	REG_HOST_SNDFIFO = 2,
	REG_HOST_SUDFIFO = 4,
	REG_HOST_RCVBC = 6,
	REG_HOST_SNDBC = 7,
	REG_HOST_USBIRQ = 13,
	REG_HOST_USBIEN = 14,
	REG_HOST_USBCTL = 15,
	REG_HOST_CPUCTL = 16,
	REG_HOST_PINCTL = 17,
	REG_HOST_REVISION = 18,
	REG_HOST_IOPINS1 = 20,
	REG_HOST_IOPINS2 = 21,
	REG_HOST_GPINIRQ = 22,
	REG_HOST_GPINIEN = 23,
	REG_HOST_GPINPOL = 24,
	REG_HOST_HIRQ = 25,
	REG_HOST_HIEN = 26,
	REG_HOST_MODE = 27,
	REG_HOST_PERADDR = 28,
	REG_HOST_HCTL = 29,
	REG_HOST_HXFR = 30,
	REG_HOST_HRSL = 31
} max3421_reg_e;

#define READ	0
#define WRITE	1



static uint8_t reg_read(uv_max3421_st *this, max3421_reg_e reg);
static void reg_write(uv_max3421_st *this, max3421_reg_e reg, uint8_t value);





static uint8_t reg_read(uv_max3421_st *this, max3421_reg_e reg) {
	uint16_t write[2] = {
			(reg << 3) | (READ << 1)
	};
	uint16_t read[2] = {};
	uv_spi_readwrite_sync(this->spi, this->ss, write, read, 8, 2);

	return read[1];
}



static void reg_write(uv_max3421_st *this, max3421_reg_e reg, uint8_t value) {
	uint16_t write[2] = {
			(reg << 3) | (WRITE << 1),
			value
	};
	uint16_t read[2] = {};
	uv_spi_readwrite_sync(this->spi, this->ss, write, read, 8, 2);
}



uv_errors_e uv_max3421_init(uv_max3421_st *this,
		max3421_mode_e mode, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e reset_io, uv_gpios_e int_io) {
	uv_errors_e ret = ERR_NONE;

	this->mode = mode;
	this->spi = spi;
	this->ss = ssel;
	this->reset_io = reset_io;
	this->int_io = int_io;

	// perform a device reset
	uv_gpio_init_output(this->reset_io, false);
	uv_rtos_task_delay(10);
	uv_gpio_set(this->reset_io, true);
	uv_rtos_task_delay(10);

	// read the device revision
	uint8_t revision = reg_read(this, REG_HOST_REVISION);
	uv_terminal_enable(TERMINAL_CAN);
	printf("revision: 0%x\n", revision);

	return ret;
}

#endif
