/*
 * uw_iap.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_MEMORY_H_
#define UW_MEMORY_H_


#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "uw_errors.h"

#ifdef LPC11C14
/// @brief: Defines the RAM size in bytes on this contoller
#define RAM_SIZE_BYTES	0x2000
#define RAM_BASE_ADDRESS 0x10000000
#elif defined(LPC1785)
/// @brief: Defines the RAM size in bytes on this contoller
#define RAM_SIZE_BYTES	64000
#define RAM_BASE_ADDRESS 0x10000000
#else
#error "Controller not defined"
#endif


/// @brief: Data type which should be used to mark the start of
/// non-volatile data section. Define a variable of this type as the
/// first variable in the data section.
typedef struct {
	/// @brief: Checksum to identify if data between start and end
	/// was uninitialized or changed from what was found in non-volatile memory.
	uint32_t start_checksum;
} uw_data_start_t;

/// @brief: Data type which should be used to mark the end of
/// non-volatile data section. Define a variable of this type as the
/// last variable in the data section.
typedef struct {
	/// @brief: Checksum to identify if data between start and end
	/// was uninitialized or changed from what was found in non-volatile memory.
	uint32_t end_checksum;
} uw_data_end_t;


/// @brief: Calls IAP commands to activate ISP mode.
/// The program execution will be stopped instantly,
/// this function should never return.
void uw_enter_ISP_mode(void);


/// @brief: Returns the approximate stack memory usage. If dynamic memory allocation
/// is not used, this gives a approximation of applications current memory usage.
/// @note: This is implementation specific. It assumes that stack starts
/// from the top of RAM and grows downward.
/// @return: Percent estimating the RAM usage (0...100)
int uw_get_stack_size(void);


/// @brief: Returns the controller specific serial number assigned by NXP in
/// the dest parameter.
/// The serial ID is 4 bytes long.
void uw_get_device_serial(unsigned int dest[4]);


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
} uw_iap_status_e;



/// @brief: Defines the possible amount of bytes to be written
typedef enum {
	IAP_BYTES_256 = 256,
	IAP_BYTES_512 = 512,
	IAP_BYTES_1024 = 1024,
	IAP_BYTES_4096 = 4096,
	IAP_BYTES_32768 = 32768,
	IAP_BYTES_COUNT
} uw_writable_amount_e;


/// @brief: Writes data to flash non-volatile application memory section. Depends on SystemCoreClock to
/// determine the clock frequency of application.
/// If saving data occurred any error, info is logged into stdout (see uw_stdout.h)
///
/// @note: Only one memory location can be saved per application!
///
/// @return: True if writing all data succeeded, false otherwise. Failing to write could
/// mean that application required to write more memory than the non-volatile application data section
/// has, or there was not enough RAM.
///
/// @param start_ptr: A pointer to a uw_data_start_t variable at the beginning of
/// destination memory where data should be copied.
/// @param end_ptr: A pointer to a uw_data_end_t variable at the end of
/// destination memory where data should be copied.
/// After copying the data, this will be used to validate if
/// non-volatile memory contained valid data or if it was undefined.
///
/// @example:
/// struct {
///		uw_data_start_t start;
///		unsigned int some_data;
///		...
///		uw_data_end_t end;
/// } data;
///	// save all data between start and end
/// uw_memory_save(&data.start, &data.end);
///
uw_errors_e uw_memory_save(uw_data_start_t *start_ptr, uw_data_end_t *end_ptr);

/// @brief: Copies data from non-volatile flash memory to another memory location,
/// usually to RAM.
/// @return: Returns true if checksum was correct and thus valid data exists in
/// non-volatile memory. Otherwise returns false and logs an error message to stdout.
///
/// @note: Regardless of the checksum validation, data from non-volatile memory will be
/// copied to the destination address. Because of this, if this function returns false,
/// the user application should reinitialize the data structure.
///
/// @param start_ptr: A pointer to a uw_data_start_t variable at the beginning of
/// destination memory where data should be copied.
/// @param end_ptr: A pointer to a uw_data_end_t variable at the end of
/// destination memory where data should be copied.
/// After copying the data, this will be used to validate if
/// non-volatile memory contained valid data or if it was undefined.
uw_errors_e uw_memory_load(uw_data_start_t *start_ptr, uw_data_end_t *end_ptr);


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
uw_iap_status_e uw_erase_and_write_to_flash(unsigned int ram_address,
		uw_writable_amount_e num_bytes, unsigned int flash_address);





/****** PROTECTED FUNCTIONS *******/
/* These functions are meant for hal library's private use and
 * the user software should'nt call them */



/// @brief: Saves the data to non-volatile memory which was saved or loaded before with
/// uw_save_non_volatile_data or uw_load_non_volatile_data.
///
/// @note: When calling uw_save_non_volatile_data or uw_load_non_volatile_data, the
/// memory addresses and data lengths are saved and this function just re-saves the same data
/// to the same location.
uw_errors_e __uw_save_previous_non_volatile_data();


/// @brief: Loads the data from non-volatile memory which was saved or loaded before with
/// uw_save_non_volatile_data or uw_load_non_volatile_data.
///
/// @note: When calling uw_save_non_volatile_data or uw_load_non_volatile_data, the
/// memory addresses and data lengths are saved and this function just re-loads the same data
/// from the same location.
uw_errors_e __uw_load_previous_non_volatile_data();


/// @brief: Clears the data from non-volatile memory
/// Does a soft memory reset. This means that the data is not really cleared, only checksums
/// are cleared.
uw_errors_e __uw_clear_previous_non_volatile_data();

#endif /* UW_MEMORY_H_ */
