/*
 * hal_iap.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef HAL_IAP_H_
#define HAL_IAP_H_


#include <stdbool.h>
#include <stdio.h>


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

/// @brief: Calls IAP commands to activate ISP mode.
/// The program execution will be stopped instantly,
/// this function should never return.
void hal_enter_ISP_mode();



/// @brief: IAP status/error codes. Writing to flash returns one of these values.
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


/// @brief: Writes data to flash non-volatile application memory section. Depends on SystemCoreClock to
/// determine the clock frequency of application.
/// If saving data occurred any error, info is logged into stdout (see hal_stdout.h)
///
/// @return: True if writing all data succeeded, false otherwise. Failing to write could
/// mean that application required to write more memory than the non-volatile application data section
/// has, or there was not enough RAM.
///
/// @param data: Pointer to data to be saved. Usually this is a pointer to struct.
/// @param length: The amount of bytes to be written. Usually sizeof(struct) is ok
/// @param checksum_ptr: Pointer to an UNINITIALIZED memory location inside data to be saved.
/// This is used to detect if uninitialized data is tried to read from non-volatile memory.
/// To disable uninitialized memory checking, set this to NULL.
/// Pointer needs to point to a least 32-bit variable.
bool hal_save_non_volatile_data(void* data, unsigned int length, uint32_t* checksum_ptr);

/// @brief: Copies data from non-volatile flash memory to another memory location,
/// usually to RAM. If checksum_ptr is not NULL, the checksum will be checked to detect
/// if non-volatile memory really contains valid data or is memory uninitialized.
///
/// @return: Returns true if checksum was correct and thus valid data exists in
/// non-volatile memory. Otherwise returns false and logs an error message to stdout.
///
/// @note: Regardless of the checksum validation, data from non-volatile memory will be
/// copied to the destination address. Because of this, if this function returns false,
/// the user application should reinitialize the data structure.
///
/// @param dest: The destination memory address where data from non-volatile memory will be copied.
/// @param length: The amount of data in bytes which will be copied.
/// @param checksum_ptr: A pointer to a 32-bit memory location inside the data structure
/// which dest is pointing. After copying the data, checksum will be checked to validate if
/// non-volatile memory contained valid data or if it was undefined. Passing NULL
/// disables the checksum checking.
bool hal_load_non_volatile_data(void* dest, unsigned int length, uint32_t* checksum_ptr);


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





/****** PROTECTED FUNCTIONS *******/
/* These functions are meant for hal library's private use and
 * the user software should'nt call them */



/// @brief: Saves the data to non-volatile memory which was saved or loaded before with
/// hal_save_non_volatile_data or hal_load_non_volatile_data.
///
/// @note: When calling hal_save_non_volatile_data or hal_load_non_volatile_data, the
/// memory addresses and data lengths are saved and this function just re-saves the same data
/// to the same location.
bool __hal_save_previous_non_volatile_data();


/// @brief: Loads the data from non-volatile memory which was saved or loaded before with
/// hal_save_non_volatile_data or hal_load_non_volatile_data.
///
/// @note: When calling hal_save_non_volatile_data or hal_load_non_volatile_data, the
/// memory addresses and data lengths are saved and this function just re-loads the same data
/// from the same location.
bool __hal_load_previous_non_volatile_data();

#endif /* HAL_IAP_H_ */
