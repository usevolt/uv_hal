/*
 * uw_can.h
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#ifndef UW_CAN_H_
#define UW_CAN_H_

#include <stdint.h>
#include <stdbool.h>
/// @file: CAN implementation on top of the HAL layer
/// Provides the basic CAN interface.
/// Note that this module takes full ownership of the CAN HAL layer interface.
/// For example on LPC11C14, this module may use all 32 CAN message objects
/// as it wants.


/// @brief: CAN message basic structure
typedef struct {
	///@brief: Message id
	uint32_t id;
	/// @brief: maximum of 8 Message data bytes
	union {
		uint8_t data_8bit[8];
		uint16_t data_16bit[4];
		uint32_t data_32bit[2];
		uint64_t data_64bit;
	};
	/// @brief: Defines how many data bytes this message has
	uint8_t data_length;
} uw_can_message_st;

/// @brief: creates a can message and returns it
uw_can_message_st uw_can_create_message(uint32_t id, uint8_t data_length, uint8_t* data);


typedef enum {
	UW_CAN_NO_ERROR,
	UW_CAN_ERROR_HARDWARE_NOT_AVAILABLE,
	UW_CAN_ERROR_HARDWARE_BUSY
} uw_can_errors_e;


/// @brief: Quick way to send a CAN message
/// @return: Enum describing if errors were found while sending the message
uw_can_errors_e uw_can_send(uint32_t id, uint8_t data_length, uint8_t* data);

/// @brief: An alternative way to send a CAN message
/// @return: Enum describing if errors were found while sending the message
uw_can_errors_e uw_can_send_message(uw_can_message_st* message);

//void uw_can_add_message_callback()



#endif /* UW_CAN_H_ */
