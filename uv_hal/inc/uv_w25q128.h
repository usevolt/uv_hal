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


#ifndef UV_HAL_INC_UV_W25Q128_H_
#define UV_HAL_INC_UV_W25Q128_H_



#include "uv_utilities.h"
#include "uv_spi.h"
#include <uv_hal_config.h>

#if CONFIG_SPI

#define CONFIG_W25Q128		1

typedef struct {
	spi_e spi;
	spi_slaves_e ssel;
} uv_w25q128_st;

#define W25Q128_SECTOR_SIZE			4096


/// @brief: Initializes the w25q128 memory module
void uv_w25q128_init(uv_w25q128_st *this, spi_e spi, spi_slaves_e ssel);


/// @brief: Reads **byte_count** bytes sychronously. The function returns when the
/// read successes.
///
/// @param address: The memory start address for the read command
/// @param dest: Pointer to the data buffer where the read data is copied
/// @param byte_count: Number of bytes to read. **dest** should be **byte_count** long.
void uv_w25q128_read_sync(uv_w25q128_st *this,
		int32_t address, void *dest, uint32_t byte_count);


/// @brief: Writes **byte_count** bytes sychronously to the memory module. The function
/// return when the write successes.
///
/// @param address: The memory start address for the write command
/// @param src: Pointer to the data buffer where the data is read
/// @param byte_count: Number of bytes to write. **src** should be **byte_count** long.
void uv_w25q128_write_sync(uv_w25q128_st *this,
		int32_t address, void *src, uint32_t byte_count);

#endif

#endif /* UV_HAL_INC_UV_W25Q128_H_ */
