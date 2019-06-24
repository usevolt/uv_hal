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

#ifndef UW_ERRORS_H_
#define UW_ERRORS_H_


#include "uv_hal_config.h"


/// @file: Defines all error messages returned by this library's
/// functions. If no error were detected, ERR_NONE is returned



typedef enum {
	/// @brief: No errors detected
	ERR_NONE							= 0,
	/// @brief: The hardware requested in one of the function's parameters is not
	/// available.
	ERR_HARDWARE_NOT_SUPPORTED			= 1,
	/// @brief: The frequency requested might be too low of too high.
	/// Use a different value.
	ERR_FREQ_TOO_HIGH_OR_TOO_LOW		= 2,
	/// @brief: The function is not yet implemented.
	ERR_NOT_IMPLEMENTED					= 3,
	/// @brief: The 1st function parameter value is invalid.
	ERR_UNSUPPORTED_PARAM1_VALUE		= 4,
	/// @brief: The 2st function parameter value is invalid.
	ERR_UNSUPPORTED_PARAM2_VALUE		= 5,
	/// @brief: The 3st function parameter value is invalid.
	ERR_UNSUPPORTED_PARAM3_VALUE		= 6,
	/// @brief: The 4st function parameter value is invalid.
	ERR_UNSUPPORTED_PARAM4_VALUE		= 7,
	/// @brief: The 5st function parameter value is invalid.
	ERR_UNSUPPORTED_PARAM5_VALUE		= 8,
	/// @brief: The 6st function parameter value is invalid.
	ERR_UNSUPPORTED_PARAM6_VALUE		= 9,
	/// @brief: End address is smaller than start address, when it should
	/// be bigger.
	ERR_END_ADDR_LESS_THAN_START_ADDR	= 10,
	/// @brief: Not enough memory on the system to fulfill the request.
	ERR_NOT_ENOUGH_MEMORY				= 11,
	/// @brief: Internal error. This is used when the function or module
	/// implements it's own error detection. Refer to the function or
	/// module for more details.
	ERR_INTERNAL						= 12,
	/// @brief: The calculated checksum doesn't match.
	ERR_START_CHECKSUM_NOT_MATCH		= 13,
	ERR_END_CHECKSUM_NOT_MATCH			= 14,
	/// @brief: No new values written since the last call.
	ERR_NO_NEW_VALUES					= 15,
	/// @brief: The function, module or peripheral is busy at the moment.
	ERR_HW_BUSY							= 16,
	/// @brief: A undefined error.
	ERR_UNDEFINED						= 17,
	/// @brief: Buffer overflow
	ERR_BUFFER_OVERFLOW					= 18,
	/// @brief: The module, function or peripheral is not responding.
	ERR_NOT_RESPONDING					= 19,
	/// @brief: Recheck the syntax of the function parameters.
	ERR_SYNTAX							= 20,
	/// @brief: The data structure or peripheral was not initialized
	ERR_NOT_INITIALIZED					= 21,
	/// @brief: The interrupt level is not defined on this hardware
	ERR_INT_LEVEL						= 22,
	/// @brief: The buffer is empty
	ERR_BUFFER_EMPTY					= 23,
	/// @brief: Indicates that some settings in uv_hal_config.h were set incorrectly.
	/// For example trying to initialise a peripheral which was not enabled in uv_hal_config.h
	ERR_INCORRECT_HAL_CONFIG			= 24,
	/// @brief: Indicates a stack overflow. This is mainly used by RTOS,
	/// when a task overflows it's own stack.
	ERR_STACK_OVERFLOW					= 25,
	/// @brief: indicates a memory allocation failure. This is mainly used by RTOS,
	/// when FreeRTOS fails to malloc memory.
	ERR_MALLOC_FAILURE					= 26,
	/// @brief: Possible misconfiguration in uv_hal_config.h. Maybe the
	/// peripheral was not enabled in uv_hal_config.h?
	ERR_HAL_CONFIG						= 27,
	/// @brief: CANopen SDO command request is unknown, SDO message is invalid.
	ERR_CANOPEN_SDO_CMD_REQUEST			= 28,
	/// @brief: CANopen node id object dictionary index specified in uv_hal_config.h
	/// was invalid
	ERR_CANOPEN_NODE_ID_ENTRY_INVALID 	= 29,
	/// @brief: CANopen producer heartbeat time object dictionary index
	/// specified in uv_hal_config.h was invalid
	ERR_CANOPEN_HEARTBEAT_ENTRY_INVALID = 30,
	/// @brief: The request couldn't be completed because the CANopen stack was
	/// in a stopped state. In stopped state only NMT and heartbeat messages
	/// are allowed.
	ERR_CANOPEN_STACK_IN_STOPPED_STATE 	= 31,
	/// @brief: Indicates that the rx message couldn't be configured to be received,
	/// because all message objects in the CAN hardware were already used.
	ERR_CAN_RX_MESSAGE_COUNT_FULL		= 32,
	/// @brief: TXPDO communication parameter object couldn't be found
	ERR_CANOPEN_PDO_COM_NOT_FOUND		= 33,
	/// @brief: TXPDO mapping parameter object couldn't be found
	ERR_CANOPEN_PDO_MAP_NOT_FOUND		= 34,
	/// @brief: TXPDO mapping parameter points to an object which doesn't exist
	ERR_CANOPEN_MAPPED_OBJECT_NOT_FOUND = 35,
	/// @brief: Object dictionary restore defaults function was NULL
	ERR_CANOPEN_RESTORE_DEFAULTS_NULL	= 36,
	/// @brief: Callback function not assigned to function, module, peripheral, etc
	ERR_CALLBACK_NOT_ASSIGNED  			= 37,
	/// @brief: Invalid data byte
	ERR_INVALID_DATA					= 38,
	/// @brief: Not acknowledge received
	ERR_NACK							= 39,
	/// @brief: Value too high
	ERR_VALUE_TOO_HIGH					= 40,
	/// @brief: index overflow
	ERR_INDEX_OVERFLOW					= 41,
	/// @brief: Message couldn't be sent for unknown reason
	ERR_MESSAGE_NOT_SENT				= 42,
	ERR_CAN_BUS_OFF						= 43,
	ERR_ABORTED							= 44,
	/// @brief: Parameter to function was null pointer
	ERR_NULL_PTR						= 45,
	/// @brief: Not found
	ERR_NOT_FOUND						= 46,
	/// @brief: Parameter was not unique
	ERR_NOT_UNIQUE 						= 47,
	/// @brief: Count exceeded
	ERR_COUNT_EXCEEDED					= 48,
	ERR_COUNT
} _uv_errors_e;
typedef unsigned int uv_errors_e;



/// @brief: Can be used to mask off the HAL module bits
#define HAL_MODULE_MASK	(0x00FFFFFF)

/// @brief: Defines for all modules in this library.
/// These are OR'red together with uv_errors_e to get the source which module
/// Returned an error. If a function returns ERR_NONE, it shouldn't OR these
/// with it!
///
/// @note: Errors log the module without shifting the bits (<< 24)
typedef enum {
	HAL_MODULE_ADC 			= (1 				<< 24),
	HAL_MODULE_CAN			= (2 				<< 24),
	HAL_MODULE_CANOPEN		= (3 				<< 24),
	HAL_MODULE_ERRORS		= (4 				<< 24),
	HAL_MODULE_FILTERS		= (5 				<< 24),
	HAL_MODULE_GPIO			= (6 				<< 24),
	HAL_MODULE_JSON			= (7 				<< 24),
	HAL_MODULE_MEMORY		= (8 				<< 24),
	HAL_MODULE_RESET		= (9 				<< 24),
	HAL_MODULE_STDOUT		= (10 				<< 24),
	HAL_MODULE_TERMINAL		= (11 				<< 24),
	HAL_MODULE_TIMER		= (12 				<< 24),
	HAL_MODULE_TYPES		= (13 				<< 24),
	HAL_MODULE_UART			= (14 				<< 24),
	HAL_MODULE_UTILITIES	= (15 				<< 24),
	HAL_MODULE_VIRTUAL_UART = (16 				<< 24),
	HAL_MODULE_WDT			= (17 				<< 24),
	HAL_MODULE_UNKNOWN		= (18 				<< 24),
	HAL_MODULE_RTOS			= (19 				<< 24),
	HAL_MODULE_I2C			= (20				<< 24),
	HAL_MODULE_SPI			= (21				<< 24),
	HAL_MODULE_EEPROM		= (22				<< 24),
	HAL_MODULE_PWM			= (23				<< 24),
	USER_MODULE_1			= (100 				<< 24),
	USER_MODULE_2			= (101 				<< 24),
	USER_MODULE_3			= (102 				<< 24),
	USER_MODULE_4			= (103 				<< 24),
	USER_MODULE_5			= (104 				<< 24),
	USER_MODULE_6			= (105 				<< 24),
	USER_MODULE_7			= (106 				<< 24),
	USER_MODULE_8			= (107 				<< 24),
	USER_MODULE_9			= (108 				<< 24),
	USER_MODULE_10			= (109 				<< 24)
} uv_hal_modules_e;


#define UV_ERR_GET(x)			(x & HAL_MODULE_MASK)
#define UV_ERR_SOURCE_GET(x) 	((x & (~HAL_MODULE_MASK)) >> 24)




/********* PRIVATE MEMBERS **********/
// Should not be used in the user application


/// @brief: Logs the error info string into stdout
uv_errors_e uv_log_error(unsigned int err);


#endif /* UW_ERRORS_H_ */


