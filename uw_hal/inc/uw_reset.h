/*
 * hal_reset.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */


#ifndef HAL_RESET_H_
#define HAL_RESET_H_


#include "uw_hal_config.h"

#include <stdbool.h>

typedef enum {
	UW_RESET_POR = 0,
	UW_RESET_EXTERNAL_PIN,
	UW_RESET_WATCHDOG,
	UW_RESET_BROWN_OUT,
	UW_RESET_SOFTWARE,
	UW_RESET_COUNT
} uw_reset_sources_e;


///@brief: returns the error id indicating what was last reset's reason
uw_reset_sources_e hal_get_reset_source(void);


/// @brief: resets the system via software reset
void hal_system_reset(bool hard_reset);


#endif /* HAL_RESET_H_ */
