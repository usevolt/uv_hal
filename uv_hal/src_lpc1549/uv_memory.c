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

#include "uv_memory.h"


#include <stdio.h>
#include <string.h>
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif
#include "uv_uart.h"
#include "uv_utilities.h"
#include "uv_wdt.h"
#include CONFIG_MAIN_H
#ifdef CONFIG_RTOS
#include "uv_rtos.h"
#endif
#if CONFIG_TARGET_LPC11C14

#define FLASH_SECTOR_SIZE					4096
#define FLASH_START_ADDRESS 				0x00000000
#define NON_VOLATILE_MEMORY_START_ADDRESS	0x00007000

#elif CONFIG_TARGET_LPC1785

#define FLASH_FIRST_SECTOR_SIZE				0x1000
#define FLASH_SECOND_SECTOR_SIZE			0x8000
#define FLASH_SECOND_SECTOR_SIZE_BEGIN		0x00010000
#define FLASH_START_ADDRESS 				0x00000000
#define NON_VOLATILE_MEMORY_START_ADDRESS	0x00001000

#elif CONFIG_TARGET_LPC1549

#define FLASH_SECTOR_SIZE					0x1000
#define FLASH_START_ADDRESS 				0x00000000
#define NON_VOLATILE_MEMORY_START_ADDRESS	0x3F000

#endif


// IAP function location, refer to user manual page 269
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785
#define IAP_LOCATION 0x1fff1ff1
#elif CONFIG_TARGET_LPC1549
#define IAP_LOCATION 0x03000205UL
#endif

#define PREPARE_SECTOR      50
#define COPY_RAM_TO_FLASH   51
#define ERASE_SECTOR        52
#define BLANK_CHECK_SECTOR  53
#define READ_PART_ID        54
#define READ_BOOT_CODE_REV  55
#define COMPARE             56
#define REINVOKE_ISP        57
#define READ_UID            58

/// typedef to IAP function
typedef void (*IAP)(unsigned int [],unsigned int[]);

#if CONFIG_NON_VOLATILE_MEMORY


const char *uv_projname = STRINGIFY(__UV_PROJECT_NAME);
const char *uv_datetime = __DATE__ " " __TIME__;

#endif




void uv_enter_ISP_mode(void) {
#if !CONFIG_TARGET_LINUX

	unsigned int command_param[5];
	unsigned int status_result[4];
	IAP iap_entry = (IAP) IAP_LOCATION;

	//disable interrupts
	__disable_irq();

	//set IAP command to invoke ISP
	command_param[0] = REINVOKE_ISP;
	//call IAP
	iap_entry( command_param, status_result );

#endif
}



void uv_get_device_serial(unsigned int dest[4]) {
#if !CONFIG_TARGET_LINUX

	unsigned int command_param[5];
	IAP iap_entry = (IAP) IAP_LOCATION;

	//disable interrupts
	__disable_irq();

	//set IAP command to read unique ID
	command_param[0] = READ_UID;
	//call IAP
	iap_entry( command_param, dest );

	// swap the bytes to different order
	unsigned int swap = dest[0];
	dest[0] = dest[3];
	dest[3] = swap;
	swap = dest[1];
	dest[1] = dest[2];
	dest[2] = swap;

	__enable_irq();

#endif
}

#if CONFIG_NON_VOLATILE_MEMORY

uv_errors_e uv_memory_save(void) {
	uv_errors_e ret = ERR_NONE;

	int length = ((unsigned int) &CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
			(unsigned int) &CONFIG_NON_VOLATILE_START;

	bool match = true;
	unsigned int i;
	for (i = 0; i < length; i++) {
		if (((uint8_t*) &CONFIG_NON_VOLATILE_START)[i] !=
				((uint8_t*) NON_VOLATILE_MEMORY_START_ADDRESS)[i]) {
			match = false;
			break;
		}
	}
	if (match) {
		ret = ERR_NONE;
	}
	else {
		printf("Flashing %u bytes\n", length);
		if (length < 0) {
			ret = ERR_END_ADDR_LESS_THAN_START_ADDR;
		}
	#if CONFIG_TARGET_LPC1785
		else if (length > IAP_BYTES_32768) {
			ret = ERR_NOT_ENOUGH_MEMORY;
		}
	#endif
		//calculate the right length
		else if (length > IAP_BYTES_4096) {
	#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549
			ret = ERR_NOT_ENOUGH_MEMORY;
	#elif CONFIG_TARGET_LPC1785
			length = IAP_BYTES_32768;
	#endif
		}
		else if (length > IAP_BYTES_1024) {
			length = IAP_BYTES_4096;
		}
		else if (length > IAP_BYTES_512) {
			length = IAP_BYTES_1024;
		}
		else if (length > IAP_BYTES_256) {
			length = IAP_BYTES_512;
		}
		else {
			length = IAP_BYTES_256;
		}
		if (ret == ERR_NONE) {

			// add the right value to data checksum
			CONFIG_NON_VOLATILE_START.project_name = uv_projname;
			CONFIG_NON_VOLATILE_START.build_date = uv_datetime;
			CONFIG_NON_VOLATILE_END.crc = uv_memory_calc_crc(&CONFIG_NON_VOLATILE_START,
					(uint32_t) &CONFIG_NON_VOLATILE_END - (uint32_t) &CONFIG_NON_VOLATILE_START);
			// note: id should be assigned by the canopen module

			uv_iap_status_e status = uv_erase_and_write_to_flash((unsigned int) &CONFIG_NON_VOLATILE_START,
					length, NON_VOLATILE_MEMORY_START_ADDRESS);
			if (status != IAP_CMD_SUCCESS) {
				ret = ERR_INTERNAL| HAL_MODULE_MEMORY;
			}
		}
	}
	return ret;
}


uv_errors_e uv_memory_load(void) {
	uv_errors_e ret = ERR_NONE;

	uint8_t* d = (uint8_t*) &CONFIG_NON_VOLATILE_START + sizeof(uv_data_start_t);
	uint8_t* source = (uint8_t*) NON_VOLATILE_MEMORY_START_ADDRESS + sizeof(uv_data_start_t);
	int length = ((unsigned int) &CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
			(unsigned int) d;

	// copy values from flash to destination
	// this requires that sizeof(char) == 1 byte.
	for (int i = 0; i < length; i++) {
		*d = *source;
		d++;
		source++;
	}

	//check crc
	if (CONFIG_NON_VOLATILE_END.crc !=
			uv_memory_calc_crc(&CONFIG_NON_VOLATILE_START,
					(uint32_t) &CONFIG_NON_VOLATILE_END - (uint32_t) &CONFIG_NON_VOLATILE_START)) {
		ret = ERR_END_CHECKSUM_NOT_MATCH;
	}
	return ret;
}



uv_iap_status_e uv_erase_and_write_to_flash(unsigned int ram_address,
		uv_writable_amount_e num_bytes, unsigned int flash_address) {
	uv_iap_status_e ret = IAP_CMD_SUCCESS;
	//find out the sectors to be erased
	unsigned int command_param[5];
	unsigned int status_result[4];

	SystemCoreClockUpdate();

	IAP iap_entry = (IAP) IAP_LOCATION;

	//disable interrupts
	__disable_irq();

	switch (num_bytes) {
	case IAP_BYTES_256:
	case IAP_BYTES_512:
	case IAP_BYTES_1024:
	case IAP_BYTES_4096:
		break;
	default:
		ret = IAP_PARAM_ERROR;
	}

	int startSection, endSection;

#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1549
	startSection = (flash_address - FLASH_START_ADDRESS) / FLASH_SECTOR_SIZE;
	endSection = (flash_address + num_bytes - FLASH_START_ADDRESS - 1) / FLASH_SECTOR_SIZE;
#elif CONFIG_TARGET_LPC1785
	if (flash_address >= FLASH_SECOND_SECTOR_SIZE_BEGIN) {
		// from sector 16 onward the section size is 32 kB
		startSection = (flash_address - FLASH_START_ADDRESS) /
				FLASH_SECOND_SECTOR_SIZE + 14;
		endSection = (flash_address + num_bytes - FLASH_START_ADDRESS - 1) /
				FLASH_SECOND_SECTOR_SIZE + 14;
	}
	else {
		// first 16 sectors the sector size is 4 kB
		startSection = (flash_address - FLASH_START_ADDRESS) / FLASH_FIRST_SECTOR_SIZE;
		endSection = (flash_address + num_bytes - FLASH_START_ADDRESS - 1) / FLASH_FIRST_SECTOR_SIZE;
	}
#endif

	if (ret == IAP_CMD_SUCCESS) {

		//prepare the flash sections for erase operation
		command_param[0] = PREPARE_SECTOR;
		command_param[1] = startSection;
		command_param[2] = endSection;

		iap_entry(command_param, status_result);

		if (status_result[0] == IAP_CMD_SUCCESS) {
			//erase the sectors
			command_param[0] = ERASE_SECTOR;
			command_param[1] = startSection;
			command_param[2] = endSection;
			command_param[3] = SystemCoreClock / 1000;
			iap_entry(command_param, status_result);

			if (status_result[0] == IAP_CMD_SUCCESS) {
				//prepare sections for writing
				command_param[0] = PREPARE_SECTOR;
				command_param[1] = startSection;
				command_param[2] = endSection;

				iap_entry(command_param, status_result);

				if (status_result[0] == IAP_CMD_SUCCESS) {
					//write the sections
					command_param[0] = COPY_RAM_TO_FLASH;
					command_param[1] = flash_address;
					command_param[2] = ram_address;
					command_param[3] = num_bytes;
					command_param[4] = SystemCoreClock / 1000;

					iap_entry(command_param, status_result);

					ret = status_result[0];
				}
				else {
					ret = status_result[0];
				}
			}
			else {
				ret = status_result[0];
			}
		}
		else {
			ret = status_result[0];
		}
	}
	//enable interrupts
	__enable_irq();

	return ret;
}




uv_errors_e uv_memory_clear(void) {
	uv_errors_e ret = ERR_NONE;

	CONFIG_NON_VOLATILE_END.crc = !CONFIG_NON_VOLATILE_END.crc;

	int length = ((unsigned int) &CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
			(unsigned int) &CONFIG_NON_VOLATILE_START;
	if (length < 0) {
		ret = ERR_END_ADDR_LESS_THAN_START_ADDR;
	}
	//calculate the right length
	else if (length > IAP_BYTES_4096) {
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	else if (length > IAP_BYTES_1024) {
		length = IAP_BYTES_4096;
	}
	else if (length > IAP_BYTES_512) {
		length = IAP_BYTES_1024;
	}
	else if (length > IAP_BYTES_256) {
		length = IAP_BYTES_512;
	}
	else {
		length = IAP_BYTES_256;
	}

	if (ret == ERR_NONE) {
		uv_iap_status_e status = uv_erase_and_write_to_flash((unsigned int) &CONFIG_NON_VOLATILE_START,
				length, NON_VOLATILE_MEMORY_START_ADDRESS);
		if (status != IAP_CMD_SUCCESS) {
			ret = ERR_INTERNAL;
		}
	}

	return ret;
}

#endif

void uv_set_id(uint16_t id) {
	CONFIG_NON_VOLATILE_START.id = id;
}

uint8_t uv_get_id() {
#if !CONFIG_TARGET_LINUX
	return *((uint8_t*)(NON_VOLATILE_MEMORY_START_ADDRESS + 8));
#else
	return 0;
#endif
}


uint16_t uv_memory_calc_crc(void *data, int32_t len) {
	uint8_t i;
	uint16_t crc = 0;
	uint8_t *d = data;

    while(--len >= 0)
    {
    	i = 8;
    	crc = crc ^ (((uint16_t)*d++) << 8);

    	do
        {
    		if (crc & 0x8000)
    		{
    			crc = crc << 1 ^ 0x1021;
    		}
    		else
    		{
    			crc = crc << 1;
    		}
        }
    	while(--i);
    }
    return crc;
}

/// @brief: Returns the project name saved in the non-volatile memory
const char *uv_memory_get_project_name() {
	return CONFIG_NON_VOLATILE_START.project_name;
}

/// @brief: Returns the project name crc value saved in the non-volatile memory
uint16_t uv_memory_get_project_id(uv_data_start_t *start_ptr) {
	return CONFIG_NON_VOLATILE_START.id;
}

/// @brief: Returns the project building date saved in the non-volatile memory
const char *uv_memory_get_project_date(uv_data_start_t *start_ptr) {
	return CONFIG_NON_VOLATILE_START.build_date;
}


uv_errors_e _uv_memory_hal_load(void) {
#if !CONFIG_TARGET_LINUX
	memcpy(&CONFIG_NON_VOLATILE_START, (void*) NON_VOLATILE_MEMORY_START_ADDRESS, sizeof(uv_data_start_t));
#endif


	return uv_memory_load();
}


