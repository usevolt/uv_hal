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

#ifndef UW_TIMER_H_
#define UW_TIMER_H_


#include "uv_hal_config.h"

#include "stdbool.h"
#include "stdint.h"
#include "uv_errors.h"
#include "uv_gpio.h"
#include <stdio.h>



typedef enum {
	TIMER0 = 0,
	TIMER1,
	TIMER2,
	TIMER3,
	TIMER_COUNT

} uv_timers_e;


/// @brief: initializes a timer/counter module as a timer but doesn't start it.
/// The timer/counter always runs at the CPU frequency.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
uv_errors_e uv_timer_init(uv_timers_e timer);










/// @brief: Starts the timer from the timer current value
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uv_timer_start(uv_timers_e timer);




/// @brief: Stops the timer without clearing the timer register
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uv_timer_stop(uv_timers_e timer);





/// @brief: Resets the timer to zero
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uv_timer_clear(uv_timers_e timer);









/// @brief: get the timer's current value in microseconds
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
int32_t uv_timer_get_us(uv_timers_e timer);










#endif /* UW_TIMER_H_ */
