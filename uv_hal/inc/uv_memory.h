/*
 * uv_iap.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_MEMORY_H_
#define UW_MEMORY_H_

#include "uv_hal_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "uv_errors.h"


#if !defined(CONFIG_NON_VOLATILE_MEMORY)
#error "CONFIG_NON_VOLATILE_MEMORY should be defined as a 1 or 0, depending\
 if non volatile memory saving needs to be enabled."
#endif
#if !defined(__UV_PROJECT_NAME)
#error "__UV_PROJECT_NAME should be defined with this project's and build's names. Usually the best way is to\
 define it in project include paths and symbols (eclipse project settings) with the ${projName},\
 '_' and ${configName} variables.\
 This way the name is automatically updated for new projects."
#endif

#if CONFIG_TARGET_LPC11C14
/// @brief: Defines the RAM size in bytes on this controller
#define RAM_SIZE_BYTES	0x2000
#define RAM_BASE_ADDRESS 0x10000000
#elif CONFIG_TARGET_LPC1785
/// @brief: Defines the RAM size in bytes on this controller
#define RAM_SIZE_BYTES	64000
#define RAM_BASE_ADDRESS 0x10000000
#else
#error "Controller not defined"
#endif


/// @brief: Data type which should be used to mark the start of
/// non-volatile data section. Define a variable of this type as the
/// first variable in the data section.
typedef struct {
	/// @brief: Project name pointer. Points to the flash memory location which
	/// contains the project name. Project name is configured with the __UV_PROJECT_NAME preprocessor symbol.
	/// With Eclipse projects the easiest way is to define __UV_PROJECT_NAME in project include paths and symbols
	/// with the ${projname} variable.
	const char *project_name;
	/// @brief: Pointer to the build date and time string. This comes from the gcc __DATE__ and __TIME__ symbols.
	const char *build_date;
	/// @brief: Project unique hash value. Can be used to identify different projects from each other.
	uint16_t project_name_crc;
	/// @brief: Checksum to identify if data between start and end
	/// was uninitialized or changed from what was found in non-volatile memory.
	uint32_t start_checksum;
} uv_data_start_t;

/// @brief: Data type which should be used to mark the end of
/// non-volatile data section. Define a variable of this type as the
/// last variable in the data section.
typedef struct {
	/// @brief: Checksum to identify if data between start and end
	/// was uninitialized or changed from what was found in non-volatile memory.
	uint32_t end_checksum;
} uv_data_end_t;


/// @brief: Calls IAP commands to activate ISP mode.
/// The program execution will be stopped instantly,
/// this function should never return.
void uv_enter_ISP_mode(void);



/// @brief: Returns the controller specific serial number assigned by NXP in
/// the dest parameter.
/// The serial ID is 4 bytes long.
void uv_get_device_serial(unsigned int dest[4]);


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
} uv_iap_status_e;



/// @brief: Defines the possible amount of bytes to be written
typedef enum {
	IAP_BYTES_256 = 256,
	IAP_BYTES_512 = 512,
	IAP_BYTES_1024 = 1024,
	IAP_BYTES_4096 = 4096,
	IAP_BYTES_32768 = 32768,
	IAP_BYTES_COUNT
} uv_writable_amount_e;


/// @brief: Returns the project name saved in the non-volatile memory
static inline const char *uv_memory_get_project_name(uv_data_start_t *start_ptr) {
	return start_ptr->project_name;
}

/// @brief: Returns the project name crc value saved in the non-volatile memory
static inline uint16_t uv_memory_get_project_name_crc(uv_data_start_t *start_ptr) {
	return start_ptr->project_name_crc;
}

/// @brief: Returns the project building date saved in the non-volatile memory
static inline const char *uv_memory_get_project_date(uv_data_start_t *start_ptr) {
	return start_ptr->build_date;
}


extern const char *uv_projname;
extern const char *uv_datetime;



/// @brief: Writes data to flash non-volatile application memory section. Depends on SystemCoreClock to
/// determine the clock frequency of application.
/// If saving data occurred any error, info is logged into stdout (see uv_stdout.h)
///
/// @note: Only one memory location can be saved per application!
///
/// @return: True if writing all data succeeded, false otherwise. Failing to write could
/// mean that application required to write more memory than the non-volatile application data section
/// has, or there was not enough RAM.
///
/// @param start_ptr: A pointer to a uv_data_start_t variable at the beginning of
/// destination memory where data should be copied.
/// @param end_ptr: A pointer to a uv_data_end_t variable at the end of
/// destination memory where data should be copied.
/// After copying the data, this will be used to validate if
/// non-volatile memory contained valid data or if it was undefined.
///
/// @example:
/// struct {
///		uv_data_start_t start;
///		unsigned int some_data;
///		...
///		uv_data_end_t end;
/// } data;
///	// save all data between start and end
/// uv_memory_save(&data.start, &data.end);
///
uv_errors_e uv_memory_save(uv_data_start_t *start_ptr, uv_data_end_t *end_ptr);

/// @brief: Copies data from non-volatile flash memory to another memory location,
/// usually to RAM.
/// @return: Returns true if checksum was correct and thus valid data exists in
/// non-volatile memory. Otherwise returns false and logs an error message to stdout.
///
/// @note: Regardless of the checksum validation, data from non-volatile memory will be
/// copied to the destination address. Because of this, if this function returns false,
/// the user application should reinitialize the data structure.
///
/// @param start_ptr: A pointer to a uv_data_start_t variable at the beginning of
/// destination memory where data should be copied.
/// @param end_ptr: A pointer to a uv_data_end_t variable at the end of
/// destination memory where data should be copied.
/// After copying the data, this will be used to validate if
/// non-volatile memory contained valid data or if it was undefined.
uv_errors_e uv_memory_load(uv_data_start_t *start_ptr, uv_data_end_t *end_ptr);


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
uv_iap_status_e uv_erase_and_write_to_flash(unsigned int ram_address,
		uv_writable_amount_e num_bytes, unsigned int flash_address);



/// @brief: Sets the CRC to the start_st structure.
/// @pre: uv_load or uv_save function should be called
void uv_set_crc(uint16_t crc);


/// @brief: Returns the CRC to the start_st structure.
/// @pre: uv_load or uv_save function should be called
uint16_t uv_get_crc();



/// @brief: Usage of these should be acoided if possible
uv_errors_e __uv_save_previous_non_volatile_data();

/// @brief: Usage of these should be acoided if possible
uv_errors_e __uv_load_previous_non_volatile_data();




#endif /* UW_MEMORY_H_ */
