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

#if CONFIG_W25Q128

#if !CONFIG_SPI
#error "SPI module is needed for W25Q128 module. Define CONFIG_SPI as 1."
#endif
#if !CONFIG_EXMEM_BUFFER_SIZE
#error "CONFIG_EXMEM_BUFFER_SIZE should define the size of memory buffer used for reading \
the data from external flash memory."
#endif

#define EXMEM_FILENAME_LEN		64

typedef struct {
	spi_e spi;
	spi_slaves_e ssel;
	/// @brief: Tells the current location for writing the data over CAN-bus
	uint32_t data_location;
} uv_w25q128_st;

#define W25Q128_SECTOR_SIZE			4096
#define W25Q128_PAGE_SIZE			256

/// @brief: Initializes the w25q128 memory module
uv_errors_e uv_w25q128_init(uv_w25q128_st *this, spi_e spi, spi_slaves_e ssel);


/// @brief: Reads **byte_count** bytes sychronously. The function returns when the
/// read successes. The maximum number of bytes that can be read equals to w25q128 sector size
///
/// @return: Pointer to the read memory location, or NULL if the reading failed
///
/// @param address: The memory start address for the read command
/// @param byte_count: Number of bytes to read. **dest** should be **byte_count** long.
/// @dest: Data destination address. This should be *byte_count* + 4 bytes long
uint8_t *uv_w25q128_read(uv_w25q128_st *this,
		int32_t address, int32_t byte_count, uint8_t *dest);


/// @brief: Writes **byte_count** bytes sychronously to the memory module. The function
/// return when the write successes.
///
/// @note: This function assumes that the memory are is untouched. That is, the memory
/// should be cleared before writing.
///
/// @return: True if wrote successful, false otherwise
///
/// @param address: The memory start address for the write command
/// @param src: Pointer to the data buffer where the data is read
/// @param byte_count: Number of bytes to write. **src** should be **byte_count** long.
bool uv_w25q128_write(uv_w25q128_st *this,
		uint32_t address, void *src, int32_t byte_count);


/// @brief: Clears the whole memory of w25q128
bool uv_w25q128_clear(uv_w25q128_st *this);


/// @brief: Clears the sector where *address* belongs to
bool uv_w25q128_clear_sector_at(uv_w25q128_st *this, uint32_t address);


/// @brief: Flash memory step function. This takes care of the CANOpen communication
void uv_w25q128_step(uv_w25q128_st *this, uint16_t step_ms);


/* External memory CAN interface */


/// @brief: External memory file descriptor data structure.
typedef struct {
	// Address of this file descriptor
	uint32_t this_addr;
	// size of the file in bytes, excluding the filename
	uint32_t file_size;
	// Since files are stored as a linked list, this holds the location of the next file
	uint32_t next_addr;
} uv_fd_st;


/// @brief: Buffer holding the data
extern uint8_t exmem_data_buffer[CONFIG_EXMEM_BUFFER_SIZE];

/// @brief: Buffer holding the file name
extern char exmem_filename_buffer[EXMEM_FILENAME_LEN];

/// @brief: Contains the file size in bytes. File is determined with *exmem_filename_buffer*
extern uint32_t exmem_file_size;

/// @brief: Write request. This should be written with
/// the byte count of data to be written. The data should be stored in
/// *exmem_data_buffer* and filename and size should be written to *exmem_filename_buffer* and
/// *exmem_file_size* prior to setting this.
extern uint32_t exmem_write_req;


/// @brief: Memory clear request.
/// Setting this to nonzero value removes all files from the external memory
extern uint8_t exmem_clear_req;

#endif


#endif /* UV_HAL_INC_UV_W25Q128_H_ */
