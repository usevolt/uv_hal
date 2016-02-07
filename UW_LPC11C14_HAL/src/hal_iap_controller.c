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
	int endSection = (flash_address + num_bytes - FLASH_START_ADDRESS) / FLASH_SECTOR_SIZE;

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
