/*
 * hal_wdt_controller.h
 *
 *  Created on: Apr 24, 2015
 *      Author: usevolt
 */

#ifndef HAL_WDT_CONTROLLER_H_
#define HAL_WDT_CONTROLLER_H_





/// @brief: Initializes watchdog timer and starts it. After this call the watchdog timer
/// is locked and cannot be stopped.
/// WDT clock source is internal RC oscillator
/// @param: time_s Desired watchdock timer cycle time in seconds. To prevent system reset
/// hal-update_wdt should be called within this time periodically.
/// The biggest possible value is 0xFFFFFF * (16 / fosc)
/// @param fosc: System oscillator frequency
void hal_init_wdt(unsigned int time_s, unsigned int fosc);


/// @brief: Loads watchdog timer counter register.
/// This function should be called periodically in order to prevent MCU from resetting
void hal_update_wdt(void);


/// @brief: Clears wdt counter register and forces a hard reset
void hal_reset_wdt(void);


#endif /* HAL_WDT_CONTROLLER_H_ */
