/*
 * uv_errors.c
 *
 *  Created on: Mar 1, 2016
 *      Author: usevolt
 */

#include "uv_errors.h"


#include "stdio.h"

uv_errors_e __uv_error;


uv_errors_e __uv_log_error(uv_errors_e err) {
	if (err) {
		printf("**** Error %u from module %u (uv_errors.h for details) **** \n\r>",
				UV_ERR_GET(err), UV_ERR_SOURCE_GET(err));
	}
	return err;
}

