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
#ifndef HAL_UV_HAL_INC_UV_MAX3421_H_
#define HAL_UV_HAL_INC_UV_MAX3421_H_


#include <uv_hal_config.h>

#if CONFIG_MAX3421

#include "uv_utilities.h"
#include "uv_spi.h"
#include "uv_gpio.h"

/// @file: MAX3421 USB-SPI controller module



typedef enum {
	// The MAX3421 operates in the peripheral mode
	MAX3421_MODE_PERIPH = 0,
	// The MAX3421 operates in the host mode
	MAX3421_MODE_HOST
} max3421_mode_e;



typedef struct {
	max3421_mode_e mode;
	uv_gpios_e reset_io;
	uv_gpios_e int_io;
	spi_e spi;
	spi_slaves_e ss;
} uv_max3421_st;


/// @brief: Initializes the MAX3421
uv_errors_e uv_max3421_init(uv_max3421_st *this,
		max3421_mode_e mode, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e reset_io, uv_gpios_e int_io);



#endif

#endif /* HAL_UV_HAL_INC_UV_MAX3421_H_ */
