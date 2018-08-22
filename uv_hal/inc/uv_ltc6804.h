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
