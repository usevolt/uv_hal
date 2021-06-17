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



#include "uv_w25q128.h"
#include <uv_rtos.h>


#if CONFIG_W25Q128

#include <uv_terminal.h>

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
		uv_terminal_enable();
	}
	printf("w25q128 manufacturer id 0x%x, device id: 0x%x\n", read[4], read[5]);

	return ret;
}


void *uv_w25q128_read(uv_w25q128_st *this,
		int32_t address, int32_t byte_count, void *dest) {
	void *ret = dest;
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
				((uint8_t*) dest)[read_count + i] = buffer[READ_CMD_LEN + i];
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

	// check for overindexing
	if (address + byte_count >= W25Q128_PAGE_COUNT * W25Q128_PAGE_SIZE) {
		ret = false;
	}
	else {
		uint32_t write_count = 0;
		while (true) {
			while (is_busy(this)) {
				uv_rtos_task_yield();
			}

			buffer[0] = CMD_WRITE_ENABLE;
			uv_spi_write_sync(this->spi, this->ssel, (uint16_t*) buffer, 8, 1);


			bool br = false;
			uint32_t offset = (address % W25Q128_PAGE_SIZE);
			uint32_t len = (byte_count > W25Q128_PAGE_SIZE) ? W25Q128_PAGE_SIZE : byte_count;
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
	uint32_t sector = address - (address % W25Q128_SECTOR_SIZE);

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
const uint32_t exmem_blocksize = CONFIG_EXMEM_BUFFER_SIZE;
char exmem_filename_buffer[EXMEM_FILENAME_LEN];
uint32_t exmem_data_offset = 0;
uint32_t exmem_file_size = 0;
int32_t exmem_write_req = 0;
uint8_t exmem_clear_req = 0;



/// @brief: Returns the filename address from the file descriptor
#define FILENAME_ADDR(fd)		(fd.this_addr + sizeof(fd))
/// @brief: Returns the data address from the file descriptor
#define DATA_ADDR(fd)			(fd.this_addr + sizeof(fd) + EXMEM_FILENAME_LEN)
/// @brief: Returns the amount of storage size needed to store the whole file
/// alongside it's filename and file descriptor
#define FILE_SIZE(fd)			(sizeof(fd) + EXMEM_FILENAME_LEN + fd.file_size)

#define EXMEM_EMPTY_ADDR		(0xFFFFFFFF)

void uv_w25q128_step(uv_w25q128_st *this, uint16_t step_ms) {
	uv_fd_st fd;
	if (exmem_write_req) {
		uv_exmem_write(this, exmem_filename_buffer, exmem_file_size,
				exmem_data_buffer, exmem_write_req, exmem_data_offset);
		exmem_write_req = 0;
	}
	else if (exmem_clear_req) {
		// clear all data from the memory
		uint32_t addr = 0;
		uv_w25q128_read(this, addr, sizeof(fd), &fd);
		while (fd.data_addr != EXMEM_EMPTY_ADDR &&
				(addr < W25Q128_SECTOR_COUNT * W25Q128_SECTOR_SIZE)) {
			for (uint32_t i = 0; i < (fd.file_size / W25Q128_SECTOR_SIZE + 1); i++) {
				uv_w25q128_clear_sector_at(this, addr + i * W25Q128_SECTOR_SIZE);
			}
			addr += (fd.file_size + W25Q128_SECTOR_SIZE - 1);
			addr -= addr % W25Q128_SECTOR_SIZE;
			uv_w25q128_read(this, addr, sizeof(fd), &fd);
		}
		exmem_clear_req = 0;
	}
	else {

	}
}


// returns an address where *filesize* of data bytes are free to be written.
// In case that memory was full, returns -1
static int32_t get_free_addr(uv_w25q128_st *this, uint32_t filesize) {
	int32_t ret = -1;
	uv_fd_st fd;
	uint32_t addr = 0;
	uint32_t free_space = 0;
	uint32_t free_space_addr = sizeof(fd);
	uv_w25q128_read(this, addr, sizeof(fd), &fd);
	while (addr < W25Q128_SECTOR_COUNT * W25Q128_SECTOR_SIZE) {
		uint32_t size = fd.file_size + W25Q128_SECTOR_SIZE - 1;
		size -= size % W25Q128_SECTOR_SIZE;
		if (size == 0) {
			size = W25Q128_SECTOR_SIZE;
		}

		if (fd.data_addr == EXMEM_DELETED_ADDR) {
			free_space += size;
		}
		else if (fd.data_addr == EXMEM_EMPTY_ADDR) {
			free_space += W25Q128_SECTOR_SIZE;
			size = W25Q128_SECTOR_SIZE;
		}
		else {
			free_space = 0;
			free_space_addr = addr + size + sizeof(fd);
		}

		addr += size;
		if (addr >= W25Q128_SECTOR_COUNT * W25Q128_SECTOR_SIZE ||
				free_space >= filesize) {
			break;
		}
		uv_w25q128_read(this, addr, sizeof(fd), &fd);
	}

	if (free_space >= filesize) {
		ret = free_space_addr;
	}
	return ret;
}


uint32_t uv_exmem_read(uv_w25q128_st *this, char *filename,
		void *dest, uint32_t max_len, uint32_t offset) {
	// try to find the file
	uint32_t ret = 0;
	uv_fd_st fd;
	if (uv_exmem_find(this, filename, &fd)) {
		// match found, copy the data to destination
		int32_t len = uv_mini(fd.file_size - offset, max_len);
		if (len > 0) {
			uv_w25q128_read(this, fd.data_addr + offset, len, dest);
		}
		else {
			len = 0;
		}
		ret = len;
	}
	return ret;
}


uint32_t uv_exmem_read_fd(uv_w25q128_st *this, uv_fd_st *fd,
		void *dest, uint32_t max_len, uint32_t offset) {
	uint32_t ret = 0;
	// copy the data to destination
	int32_t len = uv_mini(fd->file_size - offset, max_len);
	if (len > 0) {
		uv_w25q128_read(this, fd->data_addr + offset, len, dest);
	}
	else {
		len = 0;
	}
	ret = len;

	return ret;
}


uint32_t uv_exmem_write(uv_w25q128_st *this, char *filename, uint32_t filesize,
		void *src, uint32_t len, uint32_t offset) {
	uint32_t ret = 0;
	uv_fd_st fd;
	memset(fd.filename, 0, sizeof(fd.filename));
	strncpy(fd.filename, filename, strlen(filename));
	fd.file_size = filesize;
	uv_fd_st file;
	// check overindexing and thus corrupting the memory
	if (offset + len <= filesize) {
		bool found = false;
		while (uv_exmem_find(this, fd.filename, &file)) {

			uint32_t size = file.file_size + W25Q128_SECTOR_SIZE - 1;
			size -= size % W25Q128_SECTOR_SIZE;
			// data size is checked only if offset is 0, i.e. this is the first write to a file.
			// This means that an existing file is being updated
			if ((offset == 0) &&
					(filesize > size)) {
				// the file is too big to fit here. Mark this location to be deleted
				// and continue searching for a new place for the file.
				// Since the same filename shouldn't be in the memory,
				// the search continues to the end of files
				uint32_t addr = file.data_addr;
				file.data_addr = EXMEM_DELETED_ADDR;
				uv_w25q128_write(this,
						addr - sizeof(file),
						&file, sizeof(file));
			}
			else {
				if (offset == 0) {
					// start by clearing the old file if the offset was zero,
					// i.e. the first write was requested
					for (uint32_t i = 0;
							i < (((file.file_size + sizeof(file)) / W25Q128_SECTOR_SIZE) + 1);
							i++) {
						uv_w25q128_clear_sector_at(this, file.data_addr + i * W25Q128_SECTOR_SIZE);
					}
					// next disable sectors that are not used anymore
					uv_fd_st ffile = file;
					for (uint32_t i = ((fd.file_size + sizeof(fd)) / W25Q128_SECTOR_SIZE) + 1;
							i < (((file.file_size + sizeof(file)) / W25Q128_SECTOR_SIZE) + 1);
							i++) {
						ffile.file_size = W25Q128_SECTOR_SIZE - sizeof(file);
						ffile.data_addr = EXMEM_DELETED_ADDR;
						uv_w25q128_write(this,
								file.data_addr - sizeof(file) + i * W25Q128_SECTOR_SIZE,
								&ffile, sizeof(ffile));
					}
				}
				found = true;
				break;
			}
		}

		if (!found) {
			// file couldn't be found, search for a suitable place
			fd.data_addr = get_free_addr(this, filesize);
			// make sure that the sectors are cleared
			if (fd.data_addr != -1) {
				for (uint32_t i = 0;
						i < (((fd.file_size + sizeof(fd)) / W25Q128_SECTOR_SIZE) + 1);
						i++) {
					uv_w25q128_clear_sector_at(this, fd.data_addr + i * W25Q128_SECTOR_SIZE);
				}
			}

		}
		else {
			// *file* should now be pointing to free memory, or existing file which has more space
			fd.data_addr = file.data_addr;
		}
		if (fd.data_addr == -1) {
			ret = ERR_NOT_ENOUGH_MEMORY;
		}
		else {
			if (offset == 0) {
				// write the file descriptor to the start of sector
				uv_w25q128_write(this, fd.data_addr - sizeof(fd), &fd, sizeof(fd));
			}
			// write the data to file
			ret = uv_w25q128_write(this, fd.data_addr + offset, src, len) ? len : 0;
		}
	}
	else {
		ret = 0;
	}

	return ret;
}


bool uv_exmem_find(uv_w25q128_st *this, char *filename, uv_fd_st *dest) {
	bool ret = false;
	uint32_t addr = 0;
	uv_w25q128_read(this, addr, sizeof(*dest), dest);
	while (dest->data_addr != EXMEM_EMPTY_ADDR &&
			(addr < W25Q128_SECTOR_COUNT * W25Q128_SECTOR_SIZE)) {
		if (dest->data_addr != EXMEM_DELETED_ADDR) {
			if (strcmp(filename, dest->filename) == 0) {
				ret = true;
			}
		}
		if (ret == true ||
				dest->file_size == 0) {
			break;
		}
		// jump to the end of this file, aligned with the memory sector size
		addr += (dest->file_size + W25Q128_SECTOR_SIZE - 1);
		addr -= addr % W25Q128_SECTOR_SIZE;
		uv_w25q128_read(this, addr, sizeof(*dest), dest);
	}
	if (!ret) {
		// file not found, copy place for a new file to *dest*
		dest->data_addr = addr + sizeof(*dest);
	}
	return ret;
}

bool uv_exmem_index(uv_w25q128_st *this, uint32_t index, uv_fd_st *dest) {
	bool ret = false;
	uint32_t addr = 0;
	uv_w25q128_read(this, addr, sizeof(*dest), dest);
	while (dest->data_addr != EXMEM_EMPTY_ADDR &&
			(addr < W25Q128_SECTOR_COUNT * W25Q128_SECTOR_SIZE)) {
		if (index == 0) {
			if (dest->data_addr != EXMEM_DELETED_ADDR) {
				ret = true;
			}
			else {
				ret = false;
			}
			break;
		}
		index--;
		// jump to the end of this file, aligned with the memory sector size
		addr += (dest->file_size + W25Q128_SECTOR_SIZE - 1);
		addr -= addr % W25Q128_SECTOR_SIZE;
		uv_w25q128_read(this, addr, sizeof(*dest), dest);
	}
	return ret;
}


bool uv_exmem_delete(uv_w25q128_st *this, char *filename) {
	bool ret = false;
	uv_fd_st file;
	while (uv_exmem_find(this, filename, &file)) {
		ret = true;
		uint32_t size = file.file_size + W25Q128_SECTOR_SIZE - 1;
		size -= size % W25Q128_SECTOR_SIZE;

		// mark the file as deleted
		uint32_t addr = file.data_addr;
		file.data_addr = EXMEM_DELETED_ADDR;
		uv_w25q128_write(this,
				addr - sizeof(file),
				&file, sizeof(file));
	}
	return ret;
}



#endif
