/*
 * uw_errors.c
 *
 *  Created on: Mar 1, 2016
 *      Author: usevolt
 */

#include "uw_errors.h"


#include "stdio.h"

uw_errors_e __uw_error;




void __uw_log_error(unsigned int err) {

	if (err) {
		printf("\n\r**** Error %u from module %u (uw_errors.h for details) **** \n\r",
				uw_get_error, uw_get_error_source);
	}
}
