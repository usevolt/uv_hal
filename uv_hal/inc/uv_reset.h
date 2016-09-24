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
	UW_RESET_POR = 0,
	UW_RESET_EXTERNAL_PIN,
	UW_RESET_WATCHDOG,
	UW_RESET_BROWN_OUT,
	UW_RESET_SOFTWARE,
	UW_RESET_COUNT
} uv_reset_sources_e;


/// @brief: resets the system via software reset
void uv_system_reset(bool hard_reset);


#endif /* HAL_RESET_H_ */
