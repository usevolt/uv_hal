/*
 * hal_iap_controller.c
 *
 *  Created on: Feb 10, 2015
 *      Author: usenius
 */

#include <stdio.h>
#include "LPC11xx.h"
#include "hal_iap_controller.h"
#include "hal_uart_controller.h"


/// @brief: Defines the value which will be save to the checksum memory location when
/// saving data. Value should'nt be 0 or 0xFFFFFFFF, because those values are likely
/// to be found in non-initialized memory.
typedef enum {
	CHECKSUM_VALID = 0xAAAAAAAA
} hal_checksum_values_e;


// IAP function location, refer to user manual page 269
#define IAP_LOCATION 0x1fff1ff1

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

static void* last_data = NULL;
static unsigned int last_length = NULL;
static uint32_t* last_checksum_ptr = NULL;


void hal_enter_ISP_mode() {
	printf("entering ISP...\n\r");
	unsigned int command_param[5];
	unsigned int status_result[4];
	IAP iap_entry = (IAP) IAP_LOCATION;

	//disable interrupts
	__disable_irq();

	//set IAP command to invoke ISP
	command_param[0] = REINVOKE_ISP;
	//call IAP
	iap_entry( command_param, status_result );

}

bool hal_save_non_volatile_data(void* data, unsigned int length, uint32_t* checksum_ptr) {
	// take up memory locations
	last_checksum_ptr = checksum_ptr;
	last_data = data;
	last_length = length;

	//calculate the right length
	if (length > IAP_BYTES_4096) {
		printf("Error: Not enought non-volatile memory to save %u \
bytes of data from address 0x%x\n\r", length, data);
		return false;
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

	// if checksum was assigned, add the right value to it
	if (checksum_ptr) {
		*checksum_ptr = CHECKSUM_VALID;
	}

	hal_iap_status_e status = hal_erase_and_write_to_flash((unsigned int) data,
			length, NON_VOLATILE_MEMORY_START_ADDRESS, SystemCoreClock);
	if (status == IAP_CMD_SUCCESS) {
		return true;
	}
	else {
		printf("\n\rError: Saving data to non-volatile memory failed with error code %u\n\r\
Refer to hal_iap.h for different error codes.\n\r", status);
		return false;
	}
}


bool hal_load_non_volatile_data(void* dest, unsigned int length, uint32_t* checksum_ptr) {
	int i;
	char* d = (char*) dest;
	char* source = (char*) NON_VOLATILE_MEMORY_START_ADDRESS;
	// store memory locations
	last_checksum_ptr = checksum_ptr;
	last_data = dest;
	last_length = length;

	// copy values from flash to destination
	// this requires tat sizeof(char) == 1 byte.
	for (i = 0; i < length; i++) {
		*d = *source;
		d++;
		source++;
	}
	//if checksum was given, validate the data
	if (checksum_ptr && *checksum_ptr != CHECKSUM_VALID) {
		printf("\n\rError: Checksum of loaded data from non-volatile memory didn't match.\n\r\
The loaded memory locations might have been uninitialized.\n\r");
		return false;
	}
	return true;
}


bool __hal_save_previous_non_volatile_data() {
	if (!last_data) {
		printf("\n\rError: Cannot save data. data source address is not specified.\n\r\
Make sure that the application calls hal_save_non_volatile_data or\n\r\
hal_load_non_volatile_data before attempting this.\n\r");
		return false;
	}
	return hal_save_non_volatile_data(last_data, last_length, last_checksum_ptr);
}


bool __hal_load_previous_non_volatile_data() {
	if (!last_data) {
		printf("\n\rError: Cannot load data. data source address is not specified.\n\r\
Make sure that the application calls hal_save_non_volatile_data or\n\r\
hal_load_non_volatile_data before attempting this.\n\r");
		return false;
	}
	return hal_load_non_volatile_data(last_data, last_length, last_checksum_ptr);
}


hal_iap_status_e hal_erase_and_write_to_flash(unsigned int ram_address,
		hal_writable_amount_e num_bytes, unsigned int flash_address, unsigned int fosc) {
	//find out the sectors to be erased
	unsigned int command_param[5];
	unsigned int status_result[4];
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
		return IAP_NUM_BYTES_INVALID;
	}

	int startSection = (flash_address - FLASH_START_ADDRESS) / FLASH_SECTOR_SIZE;
	int endSection = (flash_address + num_bytes - FLASH_START_ADDRESS - 1) / FLASH_SECTOR_SIZE;
	printf("Writing to flash:\n\r   start section: %i\n\r   end section: %i\n\r   num bytes: %i\n\r",
			startSection, endSection, num_bytes);

	//prepare the flash sections for erase operation
	command_param[0] = PREPARE_SECTOR;
	command_param[1] = startSection;
	command_param[2] = endSection;

	iap_entry(command_param, status_result);
	if (status_result[0] != IAP_CMD_SUCCESS) {
		__enable_irq();
		return status_result[0];
	}

	//erase the sectors
	command_param[0] = ERASE_SECTOR;
	command_param[1] = startSection;
	command_param[2] = endSection;
	command_param[3] = fosc / 1000;
	iap_entry(command_param, status_result);
	if (status_result[0] != IAP_CMD_SUCCESS) {
		__enable_irq();
		return status_result[0];
	}

	//prepare sections for writing
	command_param[0] = PREPARE_SECTOR;
	command_param[1] = startSection;
	command_param[2] = endSection;

	iap_entry(command_param, status_result);
	if (status_result[0] != IAP_CMD_SUCCESS) {
		__enable_irq();
		return status_result[0];
	}

	//write the sections
	command_param[0] = COPY_RAM_TO_FLASH;
	command_param[1] = flash_address;
	command_param[2] = ram_address;
	command_param[3] = num_bytes;
	command_param[4] = fosc / 1000;

	iap_entry(command_param, status_result);




	//enable interrupts
	__enable_irq();


	return status_result[0];
}

