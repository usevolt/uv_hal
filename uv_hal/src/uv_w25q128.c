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


#if CONFIG_W25Q128


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



static bool is_busy(uv_w25q128_st *this) {
	uint16_t read[2] = {};
	uint16_t write[2] = {};
	write[0] = CMD_READ_STATUS_REGISTER_1;
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 2);
	return (read[1] & (1 << 0));
}


void uv_w25q128_init(uv_w25q128_st *this, spi_e spi, spi_slaves_e ssel) {
	this->spi = spi;
	this->ssel = ssel;

	uint16_t read[6] = {};
	uint16_t write[6] = {};
	write[0] = CMD_MANUFACTURER_DEVICE_ID;
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 6);
	printf("w25q128 manufacturer id 0x%x, device id: 0x%x\n", read[4], read[5]);
}


void uv_w25q128_read_sync(uv_w25q128_st *this,
		int32_t address, void *dest, uint32_t byte_count) {


}


void uv_w25q128_write_sync(uv_w25q128_st *this,
		int32_t address, void *src, uint32_t byte_count) {

}



#endif
