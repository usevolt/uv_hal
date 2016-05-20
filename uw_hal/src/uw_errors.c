/*
 * uw_errors.c
 *
 *  Created on: Mar 1, 2016
 *      Author: usevolt
 */

#include "uw_errors.h"


#include "stdio.h"

uw_errors_e __uw_error;

#if CONFIG_INFORMATIVE_ERRORS
const char *uw_error_strings[] = {
	"The hardware requested in one of the function's parameters is not available.",

	"The frequency requested might be too low of too high. Use a different value.",

	"The function is not yet implemented.",

	"The 1st function parameter value is invalid.",

	"The 2st function parameter value is invalid.",

	"The 3st function parameter value is invalid.",

	"The 4st function parameter value is invalid.",

	"The 5st function parameter value is invalid.",

	"The 6st function parameter value is invalid.",

	"End address is smaller than start address, when it should be bigger.",

	"Not enough memory on the system to fulfill the request.",

	"Internal error. This is used when the function or module"
	"implements it's own error detection. Refer to the function or"
	"module for more details.",

	"The calculated start checksum doesn't match.",

	"The calculated end checksum doesn't match.",

	"No new values written since the last call.",

	"The function, module or peripheral is busy at the moment.",

	"A undefined error.",

	"Buffer overflow",

	"The module, function or peripheral is not responding.",

	"Recheck the syntax of the function parameters.",

	"The data structure or peripheral was not initialized",

	"The interrupt level is not defined on this hardware",

	"The buffer is empty",

	"Indicates that some settings in uw_hal_config.h were set incorrectly."
	"For example trying to initialise a peripheral which was not enabled in uw_hal_config.h",

	"Indicates a stack overflow. This is mainly used by RTOS,"
	"when a task overflows it's own stack.",

	"indicates a memory allocation failure. This is mainly used by RTOS,"
	"when FreeRTOS fails to malloc memory.",

	"Possible misconfiguration in uw_hal_config.h. Maybe the"
	"peripheral was not enabled in uw_hal_config.h?",

	"CANopen SDO command request is unknown, SDO message is invalid.",

	"CANopen node id object dictionary index specified in uw_hal_config.h"
	"was invalid",

	"CANopen producer heartbeat time object dictionary index"
	"specified in uw_hal_config.h was invalid",

	"The request couldn't be completed because the CANopen stack was "
	"in a stopped state. In stopped state only NMT and heartbeat messages "
	"are allowed.",

	"Indicates that the rx message couldn't be configured to be received,"
	"because all message objects in the CAN hardware were already used.",

	"TXPDO communication parameter object couldn't be found",

	"TXPDO mapping parameter object couldn't be found",

	"TXPDO mapping parameter points to an object which doesn't exist"

};
#endif

void __uw_log_error(unsigned int err) {

	if (err) {
		printf("\n\r**** Error %u from module %u (uw_errors.h for details) **** \n\r",
				uw_get_error, uw_get_error_source);
#if CONFIG_INFORMATIVE_ERRORS
		if ((err & HAL_MODULE_MASK) < sizeof(uw_error_strings) / sizeof(char*)) {
			printf("***%s***", uw_error_strings[err & HAL_MODULE_MASK]);
		}
		else {
			printf("*** Index overflow in error strings***\n\r");
		}
#endif
	}
}

