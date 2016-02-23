/*
 * hal_can.h
 *
 *  Created on: Oct 26, 2015
 *      Author: usevolt
 */

#ifndef HAL_CAN_H_
#define HAL_CAN_H_



#include "stdint.h"
#include "stdbool.h"


/*
 * #warning:  on LPC11C14, HAL CAN API uses RAM addresses from 0x10000050 to 0x100000B8,
 * which should not be used by the application.
 * Make sure the linker script does not place any variables on that
 * section! (Project properties -> C/C++ Build Settings -> MCU Settings -> Memory Details)
 *
 * @note: Since 11.2.2016, CANopen interface has been removed from HAL layer and new
 * uw_can.h and uw_canopen.h interfaces have been created. You should use those instead.
*/


// loop back mode connects CAN rx and tx lines internally together
// echoing all sent messages back to receiver.
#define CAN_LOOP_BACK_MODE	0


/// CAN error defines
#define CAN_ERROR_NONE 		0x00000000UL
#define CAN_ERROR_PASS 		0x00000001UL
#define CAN_ERROR_WARN 		0x00000002UL
#define CAN_ERROR_BOFF 		0x00000004UL
#define CAN_ERROR_STUF 		0x00000008UL
#define CAN_ERROR_FORM 		0x00000010UL
#define CAN_ERROR_ACK 		0x00000020UL
#define CAN_ERROR_BIT1 		0x00000040UL
#define CAN_ERROR_BIT0 		0x00000080UL
#define CAN_ERROR_CRC 		0x00000100UL



/// @brief: All CAN message objects on LPC11C22 hardware
/// that can be used to receive or transmit messages.
/// Note: Multiple messages with same message objects should not exists
/// Message object 5 is used by CANopen NMT protocol
/// Message objects 6 and 7 are used by CANopen object dictionary.
/// Message objects 8 - 13 are used by CANopen PDO interface
typedef enum {
	MSG_OBJ_0 = 0,
	MSG_OBJ_1,
	MSG_OBJ_2,
	MSG_OBJ_3,
	MSG_OBJ_4,
	MSG_OBJ_5,
	MSG_OBJ_6,
	MSG_OBJ_7,
	MSG_OBJ_8,
	MSG_OBJ_9,
	MSG_OBJ_10,
	MSG_OBJ_11,
	MSG_OBJ_12,
	MSG_OBJ_13,
	MSG_OBJ_14,
	MSG_OBJ_15,
	MSG_OBJ_16,
	MSG_OBJ_17,
	MSG_OBJ_18,
	MSG_OBJ_19,
	MSG_OBJ_20,
	MSG_OBJ_21,
	MSG_OBJ_22,
	MSG_OBJ_23,
	MSG_OBJ_24,
	MSG_OBJ_25,
	MSG_OBJ_26,
	MSG_OBJ_27,
	MSG_OBJ_28,
	MSG_OBJ_29,
	MSG_OBJ_30,
	MSG_OBJ_31,
	MSG_OBJ_COUNT
} hal_can_msg_objs_e;
typedef uint8_t hal_can_msg_objs_t;



/// control bits for hal_can_msg_obj_st.msg_id
typedef enum {
	CAN_MSGOBJ_STD = 0x00000000UL, /* CAN 2.0a 11-bit ID */
	CAN_MSGOBJ_EXT = 0x20000000UL, /* CAN 2.0b 29-bit ID */
	CAN_MSGOBJ_DAT = 0x00000000UL, /* data frame */
	CAN_MSGOBJ_RTR = 0x40000000UL /* rtr frame */
} hl_can_message_mode_e;


/// @brief: Basic CAN message data structure
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	/// @brief: Messages ID and mode.
	/// Mode defines if message is 11 bit, 29 bit data frame or rtr frame.
	/// Control bits are OR'red with message ID, for example
	/// message.msg_id = CAN_MSGOBJ_STD | 0x123;
	/// Mode is CAN_MSGOBJ_STD by default.
	uint32_t msg_id;
	/// @brief: Message ID mask. Multiple ID's can be masked
	/// to be received into a single message object by masking
	/// out the least significant bits from message ID. This mask is AND'ed
	/// with the received messages ID => use 0x7FF is default mask for 11 bit ID's.
	uint32_t mask;
	/// @brief: Message data bytes
	uint8_t data[8];
	/// @brief: Defines how many bytes of data is sent
	uint8_t data_length;
	/// @brief: Defines which hardware message object is used for this message.
	/// Message objects should not be multiplexed among many messages.
	/// For receiving multiple messages with a single message object, use mask bits.
	hal_can_msg_objs_t msgobj;

} hal_can_msg_obj_st;





/// @brief: initializes the CAN module hardware
/// @param baudrate desired baudrate in Hz
/// @param system oscillator frequency in Hz. Used to calculate baudrate.
void hal_init_can(uint32_t baudrate, uint32_t fosc);


/// @brief: Step function for the can controller. The step function should be called periodically.
/// @param step_ms Time from the last call to this function in milliseconds.
void hal_can_step(unsigned int step_ms);


/// @brief: Sends "raw" CAN message.
/// mainly used for debugging when bypassing CANopen protocol is required
/// @param message CAN message object structure defining the whole message. Struct should
/// be statically allocated.
/// @return true if message object was free and message could be sent, false otherwise.
bool hal_can_send_raw_msg(hal_can_msg_obj_st* message);



/// @brief: Registers hardware message object as a receive object for CAN module
/// Note: Only 1 message object can be registered.
/// @param obj dedicated message object for receiving messages.
/// @param mode_id message mode (11 bit, 28 bit) and id OR'red together. See hal_can_message_mode_e
/// enum for different values.
/// @param mask Mask bits to mask out specific bits from message id. Allows multiple
/// messages with different ID's to be received with a single message object.
void hal_can_config_rx_msg_obj(hal_can_msg_objs_t obj, uint32_t mode_id, uint32_t mask);



/// @brief: Registers a callback function for received CAN messages
/// Callback function gives the received CAN message object num as a parameter
void hal_can_add_rx_callback(void (*callback_function)(hal_can_msg_obj_st*));



/// @brief: Registers a callback function for CAN errors
/// Callback function gives received errors as parameter.
void hal_can_add_error_callback(void (*callback_function)(uint32_t));


/// @brief: Checks if message object is ready to transmit data
/// @param msg_obj Number of message object to be checked
bool hal_can_msg_obj_ready(hal_can_msg_objs_e msg_obj);


/// @brief: Resets the C_CAN hardware. CAN initialization is required after calling this function
void hal_can_reset();


enum {
	CAN_ERROR_ACTIVE = 0,
	CAN_ERROR_WARNING,
	CAN_ERROR_PASSIVE,
	CAN_ERROR_BUS_OFF
};

/// @brief: Returns the CAN 2.0 specification error state. Error active is the normal state.
uint8_t hal_can_get_error_state();



#endif /* HAL_CAN_H_ */
