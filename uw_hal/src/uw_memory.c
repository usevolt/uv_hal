/*
 * uw_iap_controller.c
 *
 *  Created on: Feb 10, 2015
 *      Author: usenius
 */

#include <stdio.h>
#include <uw_memory.h>
#include "LPC11xx.h"
#include "uw_uart.h"
#include "uw_utilities.h"

/// @brief: Defines the flash memory sector size in bytes. Make sure this matches the used MCU!
/// Refer to the manual for correct value
#define FLASH_SECTOR_SIZE	4096

/// @brief: The start address of flash memory. It is important to set this right for
/// the IAP functions to determinate the right section of flash to be erased and written.
/// Refer to MCU's manual for the right value.
#define FLASH_START_ADDRESS 0x00000000

/// @brief: The last sector of flash memory is reserved for non-volatile application data storage.
/// @note: IMPORTANT: Make sure that this memory region is not used for anything else!
#define NON_VOLATILE_MEMORY_START_ADDRESS	0x00007000


/// @brief: Defines the value which will be save to the checksum memory location when
/// saving data. Value should'nt be 0 or 0xFFFFFFFF, because those values are likely
/// to be found in non-initialized memory.
typedef enum {
	CHECKSUM_VALID = 0xAAAAAAAA
} uw_checksum_values_e;


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

static uw_data_start_t* last_start = NULL;
static uw_data_end_t *last_end = NULL;


void uw_enter_ISP_mode(void) {
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


int uw_get_stack_size(void) {
	volatile int usage = UW_RAM_SIZE_BYTES - (int) &usage;
	return 100 * usage / UW_RAM_SIZE_BYTES;
}


void uw_get_device_serial(unsigned int dest[4]) {
	unsigned int command_param[5];
	IAP iap_entry = (IAP) IAP_LOCATION;

	//disable interrupts
	__disable_irq();

	//set IAP command to read unique ID
	command_param[0] = READ_UID;
	//call IAP
	iap_entry( command_param, dest );

	printf("result: 0x%x, 0x%x, 0x%x, 0x%x\n\r",
			dest[0],
			dest[1],
			dest[2],
			dest[3]);
}



bool uw_save_non_volatile_data(uw_data_start_t *start_ptr, uw_data_end_t *end_ptr) {
	// take up memory locations
	last_start = start_ptr;
	last_end = end_ptr;
	int length = ((unsigned int) end_ptr + sizeof(uw_data_end_t)) - (unsigned int) start_ptr;
	if (length < 0) {
		printf("Error: Data start memory location was after end location\n\r");
		return false;
	}
	//calculate the right length
	else if (length > IAP_BYTES_4096) {
		printf("Error: Not enought non-volatile memory to save %u \
bytes of data from address 0x%x\n\r", length, start_ptr);
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

	// add the right value to data checksum
	start_ptr->start_checksum = CHECKSUM_VALID;
	end_ptr->end_checksum = CHECKSUM_VALID;

	uw_iap_status_e status = uw_erase_and_write_to_flash((unsigned int) start_ptr,
			length, NON_VOLATILE_MEMORY_START_ADDRESS, SystemCoreClock);
	if (status == IAP_CMD_SUCCESS) {
		return true;
	}
	else {
		printf("\n\rError: Saving data to non-volatile memory failed with error code %u\n\r\
Refer to uw_iap.h for different error codes.\n\r", status);
		return false;
	}
}


bool uw_load_non_volatile_data(uw_data_start_t *start_ptr, uw_data_end_t *end_ptr) {
	int length = ((unsigned int) end_ptr + sizeof(uw_data_end_t)) - (unsigned int) start_ptr;
	int i;
	char* d = (char*) start_ptr;
	char* source = (char*) NON_VOLATILE_MEMORY_START_ADDRESS;

	// store memory locations
	last_start = start_ptr;
	last_end = end_ptr;

	// copy values from flash to destination
	// this requires that sizeof(char) == 1 byte.
	for (i = 0; i < length; i++) {
		*d = *source;
		d++;
		source++;
	}

	//check both checksums
	if (end_ptr->end_checksum != CHECKSUM_VALID || start_ptr->start_checksum) {
		printf("\n\rError: Checksum of loaded data from non-volatile memory didn't match.\n\r\
The loaded memory locations might have been uninitialized.\n\r");
		return false;
	}
	return true;
}



uw_iap_status_e uw_erase_and_write_to_flash(unsigned int ram_address,
		uw_writable_amount_e num_bytes, unsigned int flash_address, unsigned int fosc) {
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



bool __uw_save_previous_non_volatile_data() {
	if (!last_start) {
		printf("\n\rError: Cannot save data. data source address is not specified.\n\r\
Make sure that the application calls uw_save_non_volatile_data or\n\r\
uw_load_non_volatile_data before attempting this.\n\r");
		return false;
	}
	return uw_save_non_volatile_data(last_start, last_end);
}


bool __uw_load_previous_non_volatile_data() {
	if (!last_start) {
		printf("\n\rError: Cannot load data. data source address is not specified.\n\r\
Make sure that the application calls uw_save_non_volatile_data or\n\r\
uw_load_non_volatile_data before attempting this.\n\r");
		return false;
	}
	return uw_load_non_volatile_data(last_start, last_end);
}
