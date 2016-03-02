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
	ERR_NONE = 0,
	ERR_HARDWARE_NOT_SUPPORTED,
	ERR_FREQ_TOO_HIGH_OR_TOO_LOW,
	ERR_NOT_IMPLEMENTED,
	ERR_UNSUPPORTED_PARAM1_VALUE,
	ERR_UNSUPPORTED_PARAM2_VALUE,
	ERR_UNSUPPORTED_PARAM3_VALUE,
	ERR_UNSUPPORTED_PARAM4_VALUE,
	ERR_END_ADDR_LESS_THAN_START_ADDR,
	ERR_NOT_ENOUGH_MEMORY,
	ERR_INTERNAL,
	ERR_CHECKSUM_NOT_MATCH,
	ERR_NO_NEW_VALUES,
	ERR_BUSY,
	ERR_COUNT
} uw_errors_e;



/// @brief: A global error variable used for error checking
extern uw_errors_e uw_error;

/// @brief: Macro useful for error checking and passing forward the error value.
/// Used heavily inside this library.
#define uw_err_pass(function_call) if ((uw_error = function_call)) return uw_error;

/// @brief: Macro useful for error checking. Error is stored in uw_error global variable.
/// @note: Curly parenthesis are suggested after this!
#define uw_err_check(function_call) if ((uw_error = function_call))

/// @brief: Macro useful for error logging. If parameter function returns an
/// error, it's logged to stdout.
#define uw_err_log(function_call) (uw_log_error(function_call))


/// @brief: Returns a null-terminated string defining the error.
/// Can be used to log the received error.
const char *uw_get_error_str(uw_errors_e err);


/// @brief: Logs the error info string into stdout
void uw_log_error(uw_errors_e err);


#endif /* UW_ERRORS_H_ */


