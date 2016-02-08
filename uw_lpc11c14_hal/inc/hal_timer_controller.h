/*
 * hal_timer_controller.h
 *
 *  Created on: Jan 28, 2015
 *      Author: usenius
 */

#ifndef HAL_TIMER_CONTROLLER_H_
#define HAL_TIMER_CONTROLLER_H_

#include "stdbool.h"
#include "stdint.h"
#include <stdio.h>


/// @brief: Defines match outputs for pwm signals
typedef enum {
	MATCH_0 = (1 << 0),
	MATCH_1 = (1 << 1),
	MATCH_2 = (1 << 2)
} hal_timer_match_outputs_e;


/// @brief: Defines for different pwm channels
typedef enum {
	PWM_CHANNEL_0 = (1 << 0),
	PWM_CHANNEL_1 = (1 << 1),
	PWM_CHANNEL_2 = (1 << 2)
} hal_timer_pwm_channels_e;



/// @brief: initializes 16 bit timer 0
/// @param freq timer overflow loop cycle frequency in Hz
/// @param fosc system oscillator frequency in Hz
void hal_init_CT16B0(int freq, int fosc);


/// @brief: get timer 0's current value
int hal_get_CT16B0_value();

/// @brief: register callback function to be executed when delay_ms mseconds have passed
void hal_register_task_to_CT16B0(void (*callback_function)(int));

/// @brief: registers pwm callback function for timer 0.
/// @param bcallback_function function to be called
/// @param ppt 0 ... 1000 pwm value
void hal_register_pwm_CT16B0(void (*callback_function1)(int),
		void (*callback_function2)(int), uint16_t ppt);


/// @brief: Enum for all different capture logical edges possible for timers
typedef enum {
	TIMER_CAPTURE_RISING_EDGE = 0,
	TIMER_CAPTURE_FALLING_EDGE,
	TIMER_CAPTURE_BOTH_EDGES
} hal_timer_capture_edges_e;


/// @brief: Registers a callback function to CT16B0 capture 0.
/// Callback gives timer value as parameter
/// @pre: hal_init_CT16B0 has been called
void hal_set_CT16B0CAP0_callback( void (*callback_function)(uint16_t),
		hal_timer_capture_edges_e capture_edges);



/// @brief: initializes 16 bit timer 1
/// @param freq timer overflow loop cycle frequency in Hz
/// @param fosc system oscillator frequency in Hz
void hal_init_CT16B1(int freq, int fosc);


/// @brief: Initializes the 16 bit timer 1 and its pwm outputs for match register pins
/// @param pins: OR'red flags representing the desired output pins for pwm signals
/// @param freq timer overflow loop cycle frequency in Hz
/// @param fosc system oscillator frequency in Hz
void hal_init_CT16B1_pwm(hal_timer_match_outputs_e pins, int freq, int fosc);


/// @brief: Sets the desired pwm to desired duty cycle output. Duty cycle is defined as a 16 bit value
/// from 0 to 65535, 0 being output as zero and 65535 output being in a full cycle.
/// @pre: hal_init_CT16B1_pwm should have been called!
/// @param pwm_channel: Determines the pwm channel where the duty cycle value will be put
/// @param duty_cycle: The desired duty cycle from  0...65535
void hal_set_CT16B1_pwm(hal_timer_pwm_channels_e pwm_channel, uint16_t duty_cycle);


/// @brief: Returns the current value in CT16B1
uint16_t hal_get_CT16B1_value();


/// @brief: initializes ARM Cortex tick timer
/// @param freq timer loop cycle frequency in Hz
/// @param fosc system oscillator frequency in Hz
bool hal_init_tick_timer(uint32_t freq, uint32_t fosc);

/// @brief: register callback function to be executed when tick timer interrupt occurs
void hal_register_tick_timer_task(void (*callback_function_ptr)(uint32_t));

#endif /* HAL_TIMER_CONTROLLER_H_ */
