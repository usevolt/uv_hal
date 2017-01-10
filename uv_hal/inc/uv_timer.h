/*
 * uv_timer.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
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
/// To use the timer/counter module as a module, use uv_counter_init instead.
///
/// @note: This function should be called only once per application. Reinitializing
/// the timer may cause a hard fault interrupt!
///
/// @return: error enum defined in uv_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param cycle_freq: timer overflow loop cycle frequency in Hz
uv_errors_e uv_timer_init(uv_timers_e timer, float cycle_freq);






/// @brief: initializes a timer/counter module as a counter.
/// To use the timer/counter module as a module, use uv_counter_init instead.
///
/// @note: This function should be called only once per application. Reinitializing
/// the timer may cause a hard fault interrupt!
///
/// @return: error enum defined in uv_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
uv_errors_e uv_counter_init(uv_timers_e timer);






/// @brief: Starts the timer from the timer current value
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uv_timer_start(uv_timers_e timer);




/// @brief: Stops the timer without clearing the timer register
///
/// @return: error enum defined in uv_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uv_timer_stop(uv_timers_e timer);





/// @brief: Resets the timer to zero
///
/// @return: error enum defined in uv_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
void uv_timer_clear(uv_timers_e timer);
static inline void uv_counter_clear(uv_timers_e timer) { uv_timer_clear(timer); }





/// @brief: Set's the timer's overflow frequency. Can be used to change a once initialized
/// timer's frequency.
///
/// @note: Doesn't stop or clear the timer. If the timer is running when calling this function,
/// the timer increment value is updated on-the-go.
///
/// @return: error enum defined in uv_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
/// @param cycle_freq: timer overflow loop cycle frequency in Hz
uv_errors_e uv_timer_set_freq(uv_timers_e timer, float freq);






/// @brief: get the timer's current value
///
/// @return: error enum defined in uv_errors.h. Returned errors are evaluated into
/// true value, no errors evaluate into 0.
///
/// @param timer: Selects the timer which will be initialized. Available values depend
/// on hardware used.
int uv_timer_get_value(uv_timers_e timer);
static inline int uv_counter_get_value(uv_timers_e timer) { return uv_timer_get_value(timer); }










#endif /* UW_TIMER_H_ */
