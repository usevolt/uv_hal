/*
 * uw_errors.c
 *
 *  Created on: Mar 1, 2016
 *      Author: usevolt
 */

#include "uw_errors.h"
#include "stdio.h"

uw_errors_e uw_error;



const char *uw_error_strs[ERR_COUNT] = {
		"No errors",
		"Hardware not supported",
		"Frequency too high or too low",
		"Function not yet implemented",
		"Unsupported value in function parameter 1",
		"Unsupported value in function parameter 2",
		"Unsupported value in function parameter 3",
		"Unsupported value in function parameter 4",
		"End address was smaller than start address",
		"Not enough memory",
		"Internal error",
		"Checksum doesn't match",
		"No new values from last call",
		"Action is busy."
};


const char *uw_get_error_str(uw_errors_e err) {
	return (err < ERR_COUNT) ? uw_error_strs[err]: "Unknown error";

}


void uw_log_error(uw_errors_e err) {
	if (err) {
		printf("\n\r%s\n\r", uw_get_error_str(err));
	}
}
