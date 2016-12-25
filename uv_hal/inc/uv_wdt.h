/*
 * hal_wdt.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_WDT_H_
#define UW_WDT_H_


#include "uv_hal_config.h"



#if CONFIG_WDT

#if !CONFIG_WDT_CYCLE_S
#error "CONFIG_WDT_CYCLE_S should define the watchdog timer cycle time in seconds."
#endif


/// @brief: Initializes watchdog timer and starts it. After this call the watchdog timer
/// is locked and cannot be stopped.
/// WDT clock source is internal RC oscillator
void _uv_wdt_init(void);


/// @brief: Loads watchdog timer counter register.
/// This function should be called periodically in order to prevent MCU from resetting
void uv_wdt_update(void);


/// @brief: Clears wdt counter register and forces a hard reset
void uv_wdt_reset(void);


#endif

#endif /* UW_WDT_H_ */
