/*
 * uw_errors.h
 *
 *  Created on: Mar 1, 2016
 *      Author: usevolt
 */

#ifndef UW_ERRORS_H_
#define UW_ERRORS_H_



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
	ERR_CHECKSUM_NOT_MATCH				= 13,
	/// @brief: No new values written since the last call.
	ERR_NO_NEW_VALUES					= 14,
	/// @brief: The function, module or peripheral is busy at the moment.
	ERR_BUSY							= 15,
	/// @brief: A undefined error.
	ERR_UNDEFINED						= 16,
	/// @brief: Buffer overflow
	ERR_BUFFER_OVERFLOW					= 17,
	/// @brief: The module, function or peripheral is not responding.
	ERR_NOT_RESPONDING					= 18,
	/// @brief: Recheck the syntax of the function parameters.
	ERR_SYNTAX							= 19,
	/// @brief: The data structure or peripheral was not initialized
	ERR_NOT_INITIALIZED					= 20,
	/// @brief: The interrupt level is not defined on this hardware
	ERR_INT_LEVEL						= 21,
	ERR_COUNT
} _uw_errors_e;
typedef unsigned int uw_errors_e;

/// @brief: Can be used to mask off the HAL module bits
#define HAL_MODULE_MASK	(0x00FFFFFF)

/// @brief: Defines for all modules in this library.
/// These are OR'red together with uw_errors_e to get the source which module
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
} uw_hal_modules_e;


/// @brief: Generates and returns an error
/// @example:
/// // Returns a not initialized -error from custom module
/// return uw_error(ERR_NOT_INITIALIZED | USER_MODULE_1);
#define uw_err(err) (__uw_error = err)


/// @brief: Returns the error by masking of the module which caused it
#define uw_get_error  (__uw_error & HAL_MODULE_MASK)

/// @brief: Returns the module which caused the error
#define uw_get_error_source ((__uw_error & (~HAL_MODULE_MASK)) >> 24)



/// @brief: Macro useful for error checking and passing forward the error value.
#define uw_err_pass(function_call) if ((__uw_error = function_call)) { return __uw_error; }

/// @brief: Macro useful for error checking. Error is stored in uw_error global variable.
/// @note: Curly parenthesis are suggested after this!
#define uw_err_check(function_call) if ((__uw_error = function_call))

/// @brief: Macro useful for error logging. If parameter function returns an
/// error, it's logged to stdout.
#define uw_err_log(function_call) (__uw_log_error(function_call))







/********* PRIVATE MEMBERS **********/
// Should not be used in the user application


extern uw_errors_e __uw_error;


/// @brief: Logs the error to stdout and returns with it
/// @note: Used inside this library
#define __uw_err_throw(err)		__uw_log_error(__uw_error = err); return err;


/// @brief: Logs the error info string into stdout
void __uw_log_error(unsigned int err);


#endif /* UW_ERRORS_H_ */


