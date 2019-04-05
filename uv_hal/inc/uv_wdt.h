/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UW_WDT_H_
#define UW_WDT_H_


#include "uv_hal_config.h"



#if CONFIG_WDT
#if !CONFIG_WDT_CYCLE_S
#error "CONFIG_WDT_CYCLE_S should define the watchdog timer cycle time in seconds."
#endif
#endif

/// @brief: Initializes watchdog timer and starts it. After this call the watchdog timer
/// is locked and cannot be stopped.
/// WDT clock source is internal RC oscillator
void _uv_wdt_init(void);


/// @brief: Loads watchdog timer counter register.
/// This function should be called periodically in order to prevent MCU from resetting
void uv_wdt_update(void);



#endif /* UW_WDT_H_ */
