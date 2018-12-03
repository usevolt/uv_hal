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



#include "uv_w25q128.h"
#include <uv_rtos.h>


#if CONFIG_W25Q128


#define CMD_WRITE_ENABLE				0x06
#define CMD_READ_STATUS_REGISTER_1		0x05
#define CMD_WRITE_STATUS_REGISTER_1		0x01
#define CMD_READ_STATUS_REGISTER_2		0x35
#define CMD_WRITE_STATUS_REGISTER_2		0x31
#define CMD_READ_STATUS_REGISTER_3		0x15
#define CMD_WRITE_STATUS_REGISTER_3		0x11
#define CMD_CHIP_ERASE					0xC7
#define CMD_ERASE_PROGRAM_SUSPEND		0x75
#define CMD_ERASE_PROGRAM_RESUME		0x7A
#define CMD_POWER_DOWN					0xB9
#define CMD_RELEASE_POWER_DOWN			0xAB
#define CMD_MANUFACTURER_DEVICE_ID		0x90
#define CMD_JEDEC_ID					0x9F
#define CMD_GLOBAL_BLOCK_LOCK			0x7E
#define CMD_GLOBAL_BLOCK_UNLOCK			0x98
#define CMD_ENTER_QPI_MODE				0x38
#define CMD_ENABLE_RESET				0x66
#define CMD_RESET_DEVICE				0x99
#define CMD_READ_UNIQUE_ID				0x4B
#define CMD_PAGE_PROGRAM				0x02
#define CMD_QUAD_PAGE_PROGRAM			0x32
#define CMD_SECTOR_ERASE_4KB			0x20
#define CMD_BLOCK_ERASE_32KB			0x52
#define CMD_BLOCK_ERASE_64KB			0xD8
#define CMD_READ_DATA					0x03
#define CMD_FAST_READ					0x0B
#define CMD_FAST_READ_DUAL_OUTPUT		0x3B
#define CMD_FAST_READ_QUAD_OUTPUT		0x6B
#define CMD_READ_SFDP_REGISTER			0x5A
#define CMD_ERASE_SECURITY_REGISTER		0x44
#define CMD_PROGRAM_SECURITY_REGISTER	0x42
#define CMD_READ_SECURITY_REGISTER		0x48
#define CMD_INDIVIDUAL_BLOCK_LOCK		0x36
#define CMD_INDIVIDUAL_BLOCK_UNLOCK		0x39
#define CMD_READ_BLOCK_LOCK				0x3D

// The length of READ command in bytes
#define READ_CMD_LEN					4
#define WRITE_CMD_LEN					4

// RAM buffer size of W25Q128 Page but in 16-bit
static uint16_t buffer[W25Q128_PAGE_SIZE];

static bool is_busy(uv_w25q128_st *this) {
	uint16_t read[2] = {};
	uint16_t write[2] = {};
	write[0] = CMD_READ_STATUS_REGISTER_1;
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 2);
	return (read[1] & (1 << 0));
}


uv_errors_e uv_w25q128_init(uv_w25q128_st *this, spi_e spi, spi_slaves_e ssel) {
	uv_errors_e ret = ERR_NONE;
	this->spi = spi;
	this->ssel = ssel;
	this->data_location = 0;

	uint16_t read[6] = {};
	uint16_t write[6] = {};
	write[0] = CMD_MANUFACTURER_DEVICE_ID;
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 6);
	if ((read[4] != 0xef) ||
			(read[5] != 0x17)) {
		// cannot connect to the device
		ret = ERR_NOT_RESPONDING;
	}
	printf("w25q128 manufacturer id 0x%x, device id: 0x%x\n", read[4], read[5]);

	return ret;
}


uint8_t *uv_w25q128_read(uv_w25q128_st *this,
		int32_t address, int32_t byte_count, uint8_t *dest) {
	uint8_t *ret = dest;
	while (is_busy(this)) {
		uv_rtos_task_yield();
	}

	uint32_t read_count = 0;
	while (true) {
		bool br = false;
		uint16_t len = uv_mini((sizeof(buffer) / sizeof(buffer[0])) - READ_CMD_LEN, byte_count);
		buffer[0] = CMD_READ_DATA;
		buffer[1] = ((address >> 16) & 0xFF);
		buffer[2] = ((address >> 8) & 0xFF);
		buffer[3] = (address & 0xFF);

		if (uv_spi_readwrite_sync(this->spi, this->ssel, buffer, buffer, 8, len + READ_CMD_LEN)) {
			for (int32_t i = 0; i < len; i++) {
				dest[read_count + i] = buffer[READ_CMD_LEN + i];
			}
			byte_count -= len;
			read_count += len;
			address += len;
		}
		else {
			br = true;
			ret = NULL;
		}

		if (byte_count <= 0) {
			br = true;
		}
		if (br) {
			break;
		}
	}

	return ret;
}


bool uv_w25q128_write(uv_w25q128_st *this,
		uint32_t address, void *src, int32_t byte_count) {
	bool ret = true;

	uint32_t write_count = 0;
	while (true) {
		while (is_busy(this)) {
			uv_rtos_task_yield();
		}

		buffer[0] = CMD_WRITE_ENABLE;
		uv_spi_write_sync(this->spi, this->ssel, (uint16_t*) buffer, 8, 1);


		bool br = false;
		uint32_t offset = (address % W25Q128_PAGE_SIZE);
		uint32_t len = (byte_count % W25Q128_PAGE_SIZE);
		if (len + offset > W25Q128_PAGE_SIZE) {
			len -= ((len + offset) % W25Q128_PAGE_SIZE);
		}
		//writable len is now fit to w25q128 page borders

		buffer[0] = CMD_PAGE_PROGRAM;
		buffer[1] = ((address >> 16) & 0xFF);
		buffer[2] = ((address >> 8) & 0xFF);
		buffer[3] = (address & 0xFF);
		// fill buffer with the data. memcpy is not valid here, since SPI data should be 16-bits long
		for (int32_t i = 0; i < len; i++) {
			buffer[i + 4] = ((uint8_t*) src)[write_count + i];
		}
		if (!uv_spi_write_sync(this->spi, this->ssel,
				(uint16_t*) buffer, 8, WRITE_CMD_LEN + len)) {
			// problem writing the data, return false
			ret = false;
			br = true;
		}

		byte_count -= len;
		address += len;
		write_count += len;
		if (byte_count <= 0) {
			br = true;
		}
		if (br) {
			break;
		}
	}


	return ret;
}


bool uv_w25q128_clear(uv_w25q128_st *this) {
	bool ret = true;

	while (is_busy(this)) {
		uv_rtos_task_yield();
	}
	buffer[0] = CMD_WRITE_ENABLE;
	uv_spi_write_sync(this->spi, this->ssel, (uint16_t*) buffer, 8, 1);

	buffer[0] = CMD_CHIP_ERASE;
	uv_spi_write_sync(this->spi, this->ssel, buffer, 8, 1);

	return ret;
}


bool uv_w25q128_clear_sector_at(uv_w25q128_st *this, uint32_t address) {
	bool ret = true;
	while (is_busy(this)) {
		uv_rtos_task_yield();
	}


	uint32_t sector = address / W25Q128_SECTOR_SIZE;
	buffer[0] = CMD_WRITE_ENABLE;
	uv_spi_write_sync(this->spi, this->ssel, (uint16_t*) buffer, 8, 1);

	buffer[0] = CMD_SECTOR_ERASE_4KB;
	buffer[1] = ((sector >> 16) & 0xFF);
	buffer[2] = ((sector >> 8) & 0xFF);
	buffer[3] = (sector & 0xFF);
	uv_spi_write_sync(this->spi, this->ssel, buffer, 8, 4);


	return ret;
}



uint8_t exmem_data_buffer[CONFIG_EXMEM_BUFFER_SIZE];
char exmem_filename_buffer[EXMEM_FILENAME_LEN];
uint32_t exmem_file_size = 0;
uint32_t exmem_write_req = 0;
uint8_t exmem_clear_req = 0;

/// @brief: Returns a file descriptor from the filename. If the file didn't exists,
/// fd points to the last file found on the system
///
/// @return: True if the file was found, false otherwise
///
/// @param lastfd: Optional file descriptor which stores the previous fd of *fd*.
/// If *fd* was the first fd in the linked list, *lastfd* will be equal to *fd*.
static bool get_fd(const char *filename, uv_fd_st *dest, uv_fd_st *lastfd) {
	bool ret = false;

	memset(dest, 0, sizeof(uv_fd_st));

	return ret;
}

/// @brief: Writes to the memory part of the file indicated by *fd* which
/// can be found in the CANOpen object dictionary parameters
///
/// @return: True if the file was completely written, false if there should still be
/// some data to be written
static bool write_file(uv_fd_st *fd) {
	bool ret = false;

	return ret;
}


/// @brief: Returns the filename address from the file descriptor
#define FILENAME_ADDR(fd)		(fd.this_addr + sizeof(fd))
/// @brief: Returns the data address from the file descriptor
#define DATA_ADDR(fd)			(fd.this_addr + sizeof(fd) + EXMEM_FILENAME_LEN)
/// @brief: Returns the amount of storage size needed to store the whole file
/// alongside it's filename and file descriptor
#define FILE_SIZE(fd)			(sizeof(fd) + EXMEM_FILENAME_LEN + fd.file_size)
/// @brief: Returns the amount of free data space until the next file descriptor
#define FREE_SPACE_TO_NEXT(fd)	((int32_t) (fd.next_addr - fd.this_addr - \
		sizeof(fd) - EXMEM_FILENAME_LEN - fd.file_size))

void uv_w25q128_step(uv_w25q128_st *this, uint16_t step_ms) {
	uv_fd_st fd;
	if (exmem_write_req) {
		uv_fd_st lastfd;
		bool new_fd = false;
		if (get_fd(exmem_filename_buffer, &fd, &lastfd)) {
			uint32_t free_space = FREE_SPACE_TO_NEXT(fd);
			// file already exists, override it if there's enough space
			// if there was not enough space, remove the file and
			// find a new place
			if ((free_space < 0) || (free_space >= FILE_SIZE(fd))) {
				// enough space found, save the file here
				write_file(&fd);
			}
			else {
				// not enough space, forget this file descriptor and link the list again
				lastfd.next_addr = fd.next_addr;
				uv_w25q128_write(this, lastfd.this_addr, &lastfd, sizeof(lastfd));
				// request us to create a new fd
				new_fd = true;
			}
		}
		else {
			new_fd = true;
		}

		if (new_fd) {
			// new file descriptor was requested, lets create it
			// loop trough the file descriptors and find a place where there's enough space
			uv_w25q128_read(this, 0, sizeof(fd), (uint8_t*) &fd);
			lastfd = fd;
			while (fd.next_addr != 0) {
				uint32_t free_space = FREE_SPACE_TO_NEXT(fd);
				if ((free_space < 0) || (free_space > FILE_SIZE(fd))) {
					break;
				}
				lastfd = fd;
				uv_w25q128_read(this, 0, sizeof(fd), (uint8_t*) &fd);
			}
			// fd should now point to the previous file where we save the data
			uv_fd_st new;
			new.file_size = exmem_file_size;
			new.next_addr = 0;
			new.this_addr = fd.this_addr + FILE_SIZE(fd);
			uv_w25q128_write(this, new.this_addr, &new, sizeof(new));

			// write the data here
			write_file(&new);
		}

		// update the data location by the written byte count
		this->data_location += exmem_write_req;
		exmem_write_req = 0;
	}
	else if (exmem_clear_req) {
		uv_fd_st fd;
		memset(&fd, 0, sizeof(fd));
		// clears the first file descriptor. Thus, the linked list of the fd's is cleared.
		uv_w25q128_write(this, 0, &fd, sizeof(fd));
		exmem_clear_req = 0;
		this->data_location = 0;
	}
	else {

	}
}




#endif
