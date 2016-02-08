/*
 * hal_iap_controller.h
 *
 *  Created on: Feb 10, 2015
 *      Author: usenius
 *
 * HAL implementation for ISP and IAP control.
 */

#ifndef HAL_IAP_CONTROLLER_H_
#define HAL_IAP_CONTROLLER_H_

#include <stdbool.h>


/// @brief: Defines the flash memory sector size in bytes. Make sure this matches the used MCU!
/// Refer to the manual for correct value
#define FLASH_SECTOR_SIZE	4096

/// @brief: The start address of flash memory. It is important to set this right for
/// the IAP functions to determinate the right section of flash to be erased and written.
/// Refer to MCU's manual for the right value.
#define FLASH_START_ADDRESS 0x00000000

/// @brief: The last sector of flash memory is reserved for non-volatile application data storage.
#define NON_VOLATILE_MEMORY_START_ADDRESS	0x00007000

/// @brief: Calls IAP commands to activate ISP mode.
/// The program execution will be stopped instantly,
/// this function should never return.
void hal_enter_ISP_mode();



// IAP status codes
typedef enum {
    IAP_CMD_SUCCESS = 0,
	IAP_INVALID_COMMAND,
	IAP_SRC_ADDR_ERROR,
	IAP_DST_ADDR_ERROR,
	IAP_SRC_ADDR_NOT_MAPPED,
	IAP_DST_ADDR_NOT_MAPPED,
	IAP_COUNT_ERROR,
	IAP_INVALID_SECTOR,
	IAP_SECTOR_NOT_BLANK,
	IAP_SECTOR_NOT_PREPARED_FOR_WRITE_OPERATION,
	IAP_COMPARE_ERROR,
	IAP_BUSY,
	IAP_NUM_BYTES_INVALID
} hal_iap_status_e;

/// @brief: Defines the possible amount of bytes to be written
typedef enum {
	IAP_BYTES_256 = 256,
	IAP_BYTES_512 = 512,
	IAP_BYTES_1024 = 1024,
	IAP_BYTES_4096 = 4096,
	IAP_BYTES_COUNT
} hal_writable_amount_e;


/// @brief: Writes RAM data to flash. Note that the data can be written only a limited
/// amount of times, so using this function in a loop is not a good idea.
/// First prepares the given sector ready for earase and writing, then erases all data in the section
/// and lastly writes the data.
/// Executing this function may take a significant amount of time, while the interrupts are disabled.
/// Flash sections used for storing application level data should be excluded from linker
/// to make sure no application code resides on that region
///
/// @note: Do not cut off the power or reset the controller while this function executes!
///
/// @param ram_address	The start address of ram where data is read
/// @param num_bytes The amount of bytes to be written. Value must be a multiple of 256.
/// @param flash_address The start address of flash were data is written. Note that the whole sections
/// of ram where this address and the last writable address (flash_address + num_bytes) are
/// going to be erased before writing.
/// @param fosc Oscillator frequency in Hz
hal_iap_status_e hal_erase_and_write_to_flash(unsigned int ram_address,
		hal_writable_amount_e num_bytes, unsigned int flash_address, unsigned int fosc);


/// Writes data to flash non-volatile application memory section.
/// @return: True if writing all data succeeded, false otherwise. Failing to write could
/// mean that application required to write more memory than the non-volatile application data section
/// has, or there was not enough RAM.
bool save_to_non_volatile_memory(void* data, unsigned int length);




#endif /* HAL_IAP_CONTROLLER_H_ */
