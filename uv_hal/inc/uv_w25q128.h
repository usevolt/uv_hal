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
	uv_gpios_e ssel_io;
	/// @brief: Tells the current location for writing the data over CAN-bus
	uint32_t data_location;
} uv_w25q128_st;

#define W25Q128_SECTOR_SIZE			4096
#define W25Q128_SECTOR_COUNT		(W25Q128_PAGE_COUNT / 16)
#define W25Q128_PAGE_SIZE			256
#define W25Q128_PAGE_COUNT			65536

/// @brief: Initializes the w25q128 memory module
///
/// @param ssel_io: Slave select for selected SPI gpio pin that is driven manually
uv_errors_e uv_w25q128_init(uv_w25q128_st *this, spi_e spi,
		uv_gpios_e ssel_io);


/// @brief: Reads **byte_count** bytes sychronously. The function returns when the
/// read successes. The maximum number of bytes that can be read equals to w25q128 sector size
///
/// @return: Pointer to the read memory location, or NULL if the reading failed
///
/// @param address: The memory start address for the read command
/// @param byte_count: Number of bytes to read. **dest** should be **byte_count** long.
/// @dest: Data destination address. This should be *byte_count* + 4 bytes long
void *uv_w25q128_read(uv_w25q128_st *this,
		int32_t address, int32_t byte_count, void *dest);


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


/// @brief: Null file address, which represents that there is no file.
///
/// @note: This should be the cleared memory's default value
#define EXMEM_NULL_ADDR		0xFFFFFFFF
/// @brief: Deleted file address. This address represents that a file was here
/// but it has been deleted. This, however, might not be the last file in the memory
///
/// @note: This should be totally written memory's value, i.e. The value when all bits are written to.
#define EXMEM_DELETED_ADDR	0

/// @brief: External memory file descriptor data structure.
/// The filename is stored right after the fd, before the actual file
typedef struct {
	// Address of this file descriptor
	int32_t data_addr;
	// size of the file in bytes, excluding the filename
	uint32_t file_size;
	char filename[EXMEM_FILENAME_LEN];
} uv_fd_st;


/// @brief: Buffer holding the data to be written when *exmem_write_req* is set to true.
extern uint8_t exmem_data_buffer[CONFIG_EXMEM_BUFFER_SIZE];

/// @brief: Same as CONFIG_EXMEM_BUFFER_SIZE
extern const uint32_t exmem_blocksize;

/// @brief: Buffer holding the file name
extern char exmem_filename_buffer[EXMEM_FILENAME_LEN];

/// @brief: Contains the total file size. This should be written prior to settings *exmem_write_req*
extern uint32_t exmem_file_size;

/// @brief: Contains the offset of data from start of the file, where the new data is written with *exmem_write_req*
extern uint32_t exmem_data_offset;

/// @brief: Write request. This should be written with
/// the count of data to be written. The data should be stored in
/// *exmem_data_buffer* and filename and downloadable size should be written to *exmem_filename_buffer* and
/// *exmem_file_size* prior to setting this.
///
/// @example: When downloading bigger files than which fit to data buffer, first time
/// 0 should be written to this, and the next calls should download new data to data buffer and
/// write this with bigger values, until the whole file has been transferred.
extern int32_t exmem_write_req;


/// @brief: Memory clear request.
/// Setting this to nonzero value removes all files from the external memory
extern uint8_t exmem_clear_req;


/// @brief: Reads a file from external memory.
///
/// @return The number of bytes read from the memory. On success, it should match the file size,
/// or in case of when loading a big file, the file data is still to be downloaded
/// until this returns a value smaller than *max_len*
///
/// @param filename: The path and the name to the file which will be read
/// @param dest: The destination memory address where the file is read
/// @param max_len: The maximum length of the destination buffer. Maximum of this count is read.
/// @param offset: The offset byte count from the start of the file where the bytes are read.
/// This can be used if the file was bigger than the memory buffer available, by using multiple calls
/// to this function and incrementing the offset by the amount of *max_len*.
uint32_t uv_exmem_read(uv_w25q128_st *this, char *filename,
		void *dest, uint32_t max_len, uint32_t offset);


/// @brief: Reads a file from a previously fetched file descriptor *fd*. Otherwise functions
/// just like *uv_exmem_read*.
uint32_t uv_exmem_read_fd(uv_w25q128_st *this, uv_fd_st *fd,
		void *dest, uint32_t max_len, uint32_t offset);


/// Writes a file to the external flash memory
///
/// @return: The number of bytes written. On success, it should match the *len* argument.
/// If the writing failed, returns 0.
///
/// @param filename: The path and the name to the file which will be read
/// @param filesize: The total length of the file. It is important that no more than
/// this amount of data is written to the file, as this is used to reserve space for the file.
/// @param src: The source memory address where the file is read
/// @param len: The length of the source buffer, i.e. count of the data to be written on this call.
/// @param offset: The offset byte count from the start of the file where the bytes are written.
/// This can be used if the file was bigger than the memory buffer available, by using multiple calls
/// to this function and incrementing the offset by the amount of *len*.
uint32_t uv_exmem_write(uv_w25q128_st *this, char *filename, uint32_t filesize,
		void *src, uint32_t len, uint32_t offset);


/// @brief: Tries to find a file specified with *filename* from the external memory and if found,
/// copies the file desciprtor to *dest*.
///
/// @return: True if the file was found, false otherwise
///
/// @param dest: The destination pointer to where the file descriptor is copied if found. If
/// the file couldn't be found, the destination is written with a next empty address where
/// a new file could be written.
bool uv_exmem_find(uv_w25q128_st *this, char *filename, uv_fd_st *dest);


/// @brief: Copies the file descriptor to *dest* from the file which was found at index *index*.
/// Can be used to step trough all the files in the system by calling multiples times and increasing
/// the *index* always by one.
///
/// @return: True of the *index*'th file was found, false otherwise.
bool uv_exmem_index(uv_w25q128_st *this, uint32_t index, uv_fd_st *dest);


/// @brief: Deletes a file with a name of *filename*
bool uv_exmem_delete(uv_w25q128_st *this, char *filename);


#endif


#endif /* UV_HAL_INC_UV_W25Q128_H_ */
