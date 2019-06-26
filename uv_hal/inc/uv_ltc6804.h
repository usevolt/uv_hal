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


#ifndef UV_HAL_INC_UV_LTC6804_H_
#define UV_HAL_INC_UV_LTC6804_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"
#include "uv_spi.h"


// @file: LTC6804 is a LIPO balancing module with an SPI communication

#if CONFIG_LTC6804


typedef struct {
	// spi module for this
	spi_e spi;
	// slave select for this ltc6804 device
	spi_slaves_e ssel;
} uv_ltc6804_st;

#define LT6804_CELL_COUNT		12

typedef struct {
	uint16_t cell[LT6804_CELL_COUNT];
} uv_ltc6804_cells_e;


/// @brief: Initializes the LTC6804 module
void uv_ltc6804_init(uv_ltc6804_st *this, spi_e spi, spi_slaves_e ssel);


/// @brief: Returns the LIPO cell voltages
///
/// @return: Cell voltage struct with cell voltages in mv
uv_ltc6804_cells_e uv_ltc6804_get_cell_voltage_mv(uv_ltc6804_st *this);


/// @brief: Discharges the selected cell
void uv_ltc6804_cell_discharge(uv_ltc6804_st *this, uint8_t cell_index);


#endif

#endif /* UV_HAL_INC_UV_LTC6804_H_ */
