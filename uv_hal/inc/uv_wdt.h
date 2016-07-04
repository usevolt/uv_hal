/*
 * hal_wdt.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef UW_WDT_H_
#define UW_WDT_H_


#include "uv_hal_config.h"



/// @brief: Initializes watchdog timer and starts it. After this call the watchdog timer
/// is locked and cannot be stopped.
/// WDT clock source is internal RC oscillator
/// @param: time_s Desired watchdock timer cycle time in seconds. To prevent system reset
/// hal-update_wdt should be called within this time periodically.
/// The biggest possible value is 0xFFFFFF * (16 / fosc)
/// @param fosc: System oscillator frequency
void uv_wdt_init(unsigned int time_s);


/// @brief: Loads watchdog timer counter register.
/// This function should be called periodically in order to prevent MCU from resetting
void uv_wdt_update(void);


/// @brief: Clears wdt counter register and forces a hard reset
void uv_wdt_reset(void);


#endif /* UW_WDT_H_ */
