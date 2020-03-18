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

#include "uv_memory.h"


#include <stdio.h>
#include <string.h>
#include "uv_uart.h"
#include "uv_utilities.h"
#include "uv_wdt.h"
#include CONFIG_MAIN_H
#ifdef CONFIG_RTOS
#include "uv_rtos.h"
#endif

#define FLASH_SECTOR_SIZE					0x1000
#define FLASH_START_ADDRESS 				0x00000000
#define NON_VOLATILE_MEMORY_START_ADDRESS	0x3F000



// IAP function location, refer to user manual page 269
#define IAP_LOCATION 0x03000205UL

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


const char uv_projname[] = STRINGIFY(__UV_PROJECT_NAME);
const char uv_datetime[] = __DATE__ " " __TIME__;
const uint32_t uv_prog_version = __UV_PROGRAM_VERSION;
#if defined(CONFIG_SAVE_CALLBACK)
extern void CONFIG_SAVE_CALLBACK (void);
#endif

#endif



void uv_get_device_serial(unsigned int dest[4]) {

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
}

#if CONFIG_NON_VOLATILE_MEMORY

uv_errors_e uv_memory_save(void) {
	uv_errors_e ret = ERR_NONE;

#if defined(CONFIG_SAVE_CALLBACK)
		CONFIG_SAVE_CALLBACK ();
#endif

	uint32_t len = (uint32_t) &CONFIG_NON_VOLATILE_END -
			(uint32_t) &CONFIG_NON_VOLATILE_START - sizeof(uv_data_start_t);
	uint16_t crc = uv_memory_calc_crc(((uint8_t*) &CONFIG_NON_VOLATILE_START) +
			sizeof(uv_data_start_t), len);
	uint16_t hal_crc = uv_memory_calc_crc(&CONFIG_NON_VOLATILE_START, sizeof(uv_data_start_t));


	int32_t length = (((uint32_t) &CONFIG_NON_VOLATILE_END) + sizeof(uv_data_end_t)) -
			((uint32_t) &CONFIG_NON_VOLATILE_START);

	bool match = true;
	for (uint32_t i = 0; i < length; i++) {
		if (((uint8_t*) &CONFIG_NON_VOLATILE_START)[i] !=
				((uint8_t*) NON_VOLATILE_MEMORY_START_ADDRESS)[i]) {
			match = false;
			break;
		}
	}
	if (crc != CONFIG_NON_VOLATILE_END.crc ||
			hal_crc != CONFIG_NON_VOLATILE_END.hal_crc) {
		match = false;
	}

	if (match) {
		ret = ERR_NONE;
	}
	else {
		printf("Flashing %u bytes\n", (int) length);
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

			// add the right value to data checksum
			CONFIG_NON_VOLATILE_START.project_name = uv_projname;
			CONFIG_NON_VOLATILE_START.build_date = uv_datetime;
			CONFIG_NON_VOLATILE_END.hal_crc = hal_crc;
			CONFIG_NON_VOLATILE_END.crc = crc;

			uv_iap_status_e status = uv_erase_and_write_to_flash((uint32_t) &CONFIG_NON_VOLATILE_START,
					length, NON_VOLATILE_MEMORY_START_ADDRESS);
			if (status != IAP_CMD_SUCCESS) {
				ret = ERR_INTERNAL| HAL_MODULE_MEMORY;
			}
		}
	}
	return ret;
}



uv_errors_e uv_memory_load(memory_scope_e scope) {
	uv_errors_e ret = ERR_NONE;

	uint8_t* d;
	uint8_t* source;
	int32_t length;

	if (scope & MEMORY_COM_PARAMS) {
		d = (uint8_t*) & CONFIG_NON_VOLATILE_START;
		source = (uint8_t*) NON_VOLATILE_MEMORY_START_ADDRESS;
	}
	else {
		d = ((uint8_t*) & CONFIG_NON_VOLATILE_START) + sizeof(uv_data_start_t);
		source = ((uint8_t*) NON_VOLATILE_MEMORY_START_ADDRESS) + sizeof(uv_data_start_t);
	}

	if (scope & MEMORY_APP_PARAMS) {
		length = ((uint32_t) & CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
				(uint32_t) d;
	}
	else {
		length = sizeof(uv_data_start_t);
	}

	// copy values from flash to destination
	memcpy(d, source, length);

	source = (uint8_t *) NON_VOLATILE_MEMORY_START_ADDRESS +
			((uint32_t) & CONFIG_NON_VOLATILE_END - (uint32_t) & CONFIG_NON_VOLATILE_START);

	if ((unsigned int) source + sizeof(uv_data_end_t) >
			NON_VOLATILE_MEMORY_START_ADDRESS + FLASH_SECTOR_SIZE) {
		// the non-volatile data was greater than Flash sector size.
		// This is not allowed
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	else {
		// make sure to copy the end structure, since it contains the crc checksums
		memcpy(& CONFIG_NON_VOLATILE_END, (uint8_t *) NON_VOLATILE_MEMORY_START_ADDRESS +
				((uint32_t) & CONFIG_NON_VOLATILE_END - (uint32_t) & CONFIG_NON_VOLATILE_START),
				sizeof(uv_data_end_t));

		//check crc
		if (scope & MEMORY_APP_PARAMS) {
			uint32_t len = (uint32_t) & CONFIG_NON_VOLATILE_END -
					(uint32_t) & CONFIG_NON_VOLATILE_START - sizeof(uv_data_start_t);
			uint16_t crc = uv_memory_calc_crc(((uint8_t*) & CONFIG_NON_VOLATILE_START) +
					sizeof(uv_data_start_t), len);

			if (CONFIG_NON_VOLATILE_END.crc != crc) {
				ret = ERR_END_CHECKSUM_NOT_MATCH;
			}
		}
		if (scope & MEMORY_COM_PARAMS) {
			// calculate the HAL checksum and compare it to the loaded value
			uint16_t crc = uv_memory_calc_crc(& CONFIG_NON_VOLATILE_START, sizeof(uv_data_start_t));
			if (crc != CONFIG_NON_VOLATILE_END.hal_crc) {
				// hal crc didn't match, which means that we have loaded invalid settings.
				// Revert the HAL system defaults
				_uv_rtos_hal_reset();
				ret = ERR_START_CHECKSUM_NOT_MATCH;
			}
		}
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

	startSection = (flash_address - FLASH_START_ADDRESS) / FLASH_SECTOR_SIZE;
	endSection = (flash_address + num_bytes - FLASH_START_ADDRESS - 1) / FLASH_SECTOR_SIZE;

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




uv_errors_e uv_memory_clear(memory_scope_e scope) {
	uv_errors_e ret = ERR_NONE;

	if (scope & MEMORY_APP_PARAMS) {
		CONFIG_NON_VOLATILE_END.crc = !CONFIG_NON_VOLATILE_END.crc;
	}
	if (scope & MEMORY_COM_PARAMS) {
		CONFIG_NON_VOLATILE_END.hal_crc = !CONFIG_NON_VOLATILE_END.hal_crc;
	}

	int32_t length = ((uint32_t) &CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
			(uint32_t) &CONFIG_NON_VOLATILE_START;
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
		uv_iap_status_e status = uv_erase_and_write_to_flash((uint32_t) &CONFIG_NON_VOLATILE_START,
				length, NON_VOLATILE_MEMORY_START_ADDRESS);
		if (status != IAP_CMD_SUCCESS) {
			ret = ERR_INTERNAL;
		}
	}

	return ret;
}

#endif



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





uint32_t uv_memory_get_can_baudrate(void) {
	return CONFIG_NON_VOLATILE_START.can_baudrate;
}

void uv_memory_set_can_baudrate(uint32_t baudrate) {
	CONFIG_NON_VOLATILE_START.can_baudrate = baudrate;
}


void uv_memory_set_bootloader_wait_time(uint32_t value_ms) {
	CONFIG_NON_VOLATILE_START.bootloader_wait_time = value_ms * (0x3B000 / 1000);
}

uint32_t uv_memory_get_bootloader_wait_time(void) {
	return (CONFIG_NON_VOLATILE_START.bootloader_wait_time / (0x3B000 / 1000));
}


