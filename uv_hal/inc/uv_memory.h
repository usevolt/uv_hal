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

#ifndef UW_MEMORY_H_
#define UW_MEMORY_H_

#include "uv_hal_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include "uv_errors.h"
#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif
#if CONFIG_W25Q128
#include "uv_w25q128.h"
#endif


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
#if !defined(CONFIG_INTERFACE_REVISION)
#error "CONFIG_INTERFACE_REVISION should define the CAN interface revision number which starts from 1 and\
 rises every time the CAN interface is changed. It is used to support older interfaces on CAN-bus devices.\
 If the device dosn't use interface revisioning, set this to 0."
#endif
#if !defined(CONFIG_APP_ST)
#error "CONFIG_APP_ST should define the main user application structure type and name."
#endif
#if !defined(CONFIG_NON_VOLATILE_START)
#error "CONFIG_NON_VOLATILE_START should defined the uv_data_start_st variable in this application"
#endif
#if !defined(CONFIG_NON_VOLATILE_END)
#error "CONFIG_NON_VOLATILE_END should defined the uv_data_end_st variable in this application"
#endif
#if !defined(CONFIG_MAIN_H)
#error "CONFIG_MAIN_H should define the name of the application main header file as a string"
#endif

#if CONFIG_TARGET_LPC11C14
/// @brief: Defines the RAM size in bytes on this controller
#define RAM_SIZE_BYTES	0x2000
#define RAM_BASE_ADDRESS 0x10000000
#elif CONFIG_TARGET_LPC1785
/// @brief: Defines the RAM size in bytes on this controller
#define RAM_SIZE_BYTES	64000
#define RAM_BASE_ADDRESS 0x10000000
#elif CONFIG_TARGET_LPC1549
/// @brief: Defines the RAM size in bytes on this controller
#define RAM_SIZE_BYTES	0x9000
#define RAM_BASE_ADDRESS 0x2000000
#elif CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
#define RAM_SIZE_BYTES	1
#define RAM_BASE_ADDRESS 1
#else
#warning "Controller not defined"
#endif


/// @brief: Contains the RAM address of the data structure which is used
/// to pass a received can message. This is used with CANopen compatible bootloader.
/// When the boot process starts, this address is loaded with that CAN message and
/// bootloader is triggered.
#define UV_BOOTLOADER_DATA_ADDR			((void*) (RAM_BASE_ADDRESS + RAM_SIZE_BYTES - sizeof(uv_can_msg_st)))
#define UV_BOOTLOADER_DATA_LEN			(sizeof(uv_can_msg_st))

typedef uint32_t uv_bootloader_wait_t;


/// @brief: Data type which should be used to mark the start of
/// non-volatile data section. Define a variable of this type as the
/// first variable in the data section.
typedef struct __attribute__((packed)) {
	// *****************************************************************************
	// NOTE: THE BYTE ORDER OF THIS IS HARDCODED IN UV_BOOTLOADER. DO NOT CHANGE IT!
	//	****************************************************************************

	// deprecated proj name field. Memory allocation required for uv_bootloader.
	uint32_t _reserved;
	// Deprecated proj build date field. Memory allocation required for uv_bootloader.
	uint32_t _reserved2;
	/// @brief: Project unique ID. Usually the same as a CANopen node-id
	uint16_t id;
	uint16_t _reserved3;
	// @brief: CAN baudrate. Defaults to 250000. UV bootloader uses this when booting.
	uint32_t can_baudrate;
	/// @brief: UV bootloader wait time in processor loops
	uv_bootloader_wait_t bootloader_wait_time;
#if CONFIG_CANOPEN
	// non-volatile data for canopen
	uv_canopen_non_volatile_st canopen_data;
#endif
} uv_data_start_t;

/// @brief: Data type which should be used to mark the end of
/// non-volatile data section. Define a variable of this type as the
/// last variable in the data section.
typedef struct {
	// crc sum for checking the HAL-library specific non-volatile data.
	uint16_t hal_crc;
	/// @brief: Checksum to identify if data between start and end
	/// was uninitialized or changed from what was found in non-volatile memory.
	uint16_t crc;
} uv_data_end_t;


/// @brief: Processor dependent macro for defining the bootloader_wait_time in milliseconds
void uv_memory_set_bootloader_wait_time(uint32_t value_ms);

/// @brief: Returns the bootloader wait time in milliseconds
uint32_t uv_memory_get_bootloader_wait_time(void);



/// @brief: Returns the controller specific serial number assigned by NXP in
/// the dest parameter.
/// The serial ID is 4 bytes long.
void uv_get_device_serial(unsigned int dest[4]);


/// @brief: IAP status/error codes. Writing to flash returns one of these values.
#if CONFIG_TARGET_LPC11C14 || CONFIG_TARGET_LPC1785
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
	IAP_PARAM_ERROR
} uv_iap_status_e;
#else
typedef int uv_iap_status_e;
#endif


typedef enum {
	IAP_BYTES_256 = 256,
	IAP_BYTES_512 = 512,
	IAP_BYTES_1024 = 1024,
	IAP_BYTES_4096 = 4096,
	IAP_BYTES_32768 = 32768,
	IAP_BYTES_COUNT
} uv_writable_amount_e;


/// @brief: Returns the project name saved in the non-volatile memory
const char *uv_memory_get_project_name();

/// @brief: Returns the project name crc value saved in the non-volatile memory
uint16_t uv_memory_get_project_id(uv_data_start_t *start_ptr);

/// @brief: Returns the project building date saved in the non-volatile memory
const char *uv_memory_get_project_date(uv_data_start_t *start_ptr);

/// @brief: Returns the CAN bus baudrate
uint32_t uv_memory_get_can_baudrate(void);

/// @brief: Sets the CAN baudrate. Note that the memory should usually be saved after this
/// if the value should be non-volatile.
void uv_memory_set_can_baudrate(uint32_t baudrate);


extern const char uv_projname[];
extern const char uv_datetime[];
extern const uint32_t uv_prog_version;


/// @brief: Defines the scope of possible load / erase operations
typedef enum {
	// The operation affect only communication specific parameters (i.e. HAL params)
	MEMORY_COM_PARAMS = 0x1,
	// The operation affects only applications parameters
	MEMORY_APP_PARAMS = 0x2,
	// The operation affects all non-volatile parameters
	MEMORY_ALL_PARAMS = MEMORY_COM_PARAMS | MEMORY_APP_PARAMS
} memory_scope_e;


/// @brief: Writes data to flash non-volatile application memory section. Depends on SystemCoreClock to
/// determine the clock frequency of application.
/// If while saving data occurred any error, info is logged into stdout (see uv_stdout.h)
///
/// @note: Only one memory location can be saved per application!
///
/// @return: True if writing all data succeeded, false otherwise. Failing to write could
/// mean that application required to write more memory than the non-volatile application data section
/// has, or there was not enough RAM.
///
uv_errors_e uv_memory_save(void);


/// @brief: Copies data from non-volatile flash memory to another memory location,
/// usually to RAM.
/// @return: Returns true if checksum was correct and thus valid data exists in
/// non-volatile memory. Otherwise returns false and logs an error message to stdout.
///
/// @note: Regardless of the checksum validation, data from non-volatile memory will be
/// copied to the destination address. Because of this, if this function returns false,
/// the user application should reinitialize the data structure.
///
uv_errors_e uv_memory_load(memory_scope_e scope);


/// @brief: Clears the previously save non-volatile data.
///
/// @note: Despite the value of *scope*, this always saves the non-volatile settings.
/// *Scope* just clears the CRC-filed in either app or communication settings, resulting
/// in possible hazardous situation where modified application settings are saved when
/// clearing communication settings, or vice versa.
uv_errors_e uv_memory_clear(memory_scope_e scope);



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





/// @brief: Calculates and returns a cyclic redundancy check value from the given data
uint16_t uv_memory_calc_crc(void *data, int32_t len);



#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
/// @brief: Sets the filepath for the non-volatile memory location.
/// Defaults to *./$(PROJECT_NAME).nvconf
void uv_memory_set_nonvol_filepath(char *filepath);


/// @brief: Returns the nonvolatile memory file name
char *uv_memory_get_nonvol_filepath(void);
#endif




#endif /* UW_MEMORY_H_ */
