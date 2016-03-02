/*
 * uw_utilities.h
 * Reusable utility functions
 *
 *  Created on: Feb 18, 2015
 *      Author: usenius
 */

#ifndef UW_UTILITIES_H_
#define UW_UTILITIES_H_

#include "uw_types.h"
#include "uw_can.h"
#include <stdbool.h>



#define GetInt16(hibyte,lobyte)		(((uint16_t) hibyte << 8) + (uint16_t) lobyte)




/// @brief: returns the masked value of 'value'
/// @example: GET_MASKED(0b111000, 0b100001);	// returns 0b100000;
#define GET_MASKED(value, mask)		(value & mask)

int uw_atoi (const char* str);


/// @brief: Initializes a delay.
/// @param delay_ms The desired length of the delay
/// @param p A pointer to variable which will hold the current delay count
void uw_start_delay(unsigned int delay_ms, int* p);

/// @brief: Delay function. Can be used to create different delays in a discrete cyclical step function system.
/// @param delay_ms The length of the delay in ms.
/// @param step_ms The duration between cyclic calls in ms.
/// @param p Pointer to a delay counter variable. Variable's value is increased every step and
/// @pre: uw_start_delay should have been called
/// when it exceeds delay_ms, true is returned.
/// @example:
/// 	static int p;
/// 	uw_start_delay(1500, &p);
/// 	while (true) {
///			if (uw_delay(1, &p)) {
///				...
///			}
/// 	}
bool uw_delay(unsigned int step_ms, int* p);


/// @brief:  function to ease debug console coding for on/off parameters
/// arg:     command line argument, eg args[0]
/// current: current value of the on/off parameter
/// param:   name of the param for console feedback print
/// example:
///    Worklights = Debug_ParamOnOff (args[0], Worklights, "work lights")
///    parses command "work off"
///    prints "turning work lights off"
bool Debug_ParamOnOff (const char* arg, bool current, const char* param);


/// @brief:  prints can message contents in debug console
void Debug_PrintMessage (uw_can_message_st* msg);



/// @brief: Set's the user's application pointer.
/// User can set a pointer to any variable which will be passed to all this library's
/// callback functions. This makes it easier to write object oriented code, since
/// the callback function has the object's instace as a parameter.
void uw_set_application_ptr(void *ptr);



/// @brief: Returns this controller's name as a null-terminated string
char *uw_get_hardware_name();


/**** PROTECTED FUNCTIONS ******/
/* These are meant only for HAL library's internal usage */

/// @brief: Get's the user's application pointer
/// User can set a pointer to any variable which will be passed to all this library's
/// callback functions. This makes it easier to write object oriented code.
void *__uw_get_user_ptr();


#endif /* UW_UTILITIES_H_ */
