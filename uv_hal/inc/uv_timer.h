/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
uint32_t uv_timer_get_us(uv_timers_e timer);










#endif /* UW_TIMER_H_ */
