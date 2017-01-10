/*
 * hal_reset.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */


#ifndef HAL_RESET_H_
#define HAL_RESET_H_


#include "uv_hal_config.h"

#include <stdbool.h>

typedef enum {
	UV_RESET_POR = 0,
	UV_RESET_EXTERNAL,
	UV_RESET_WATCHDOG,
	UV_RESET_BROWN_OUT,
	UV_RESET_SOFTWARE,
	UV_RESET_LOCKUP,
	UV_RESET_COUNT
} uv_reset_sources_e;


/// @brief: resets the system via software reset
void uv_system_reset();



#endif /* HAL_RESET_H_ */
