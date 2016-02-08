/*
 * uw_can_controller.h
 *
 *  Created on: Jan 27, 2015
 *      Author: usenius
 */

#ifndef HAL_CAN_CONTROLLER_H_
#define HAL_CAN_CONTROLLER_H_

#include "stdint.h"
#include "stdbool.h"


/*
 #warning HAL CAN API uses RAM addresses from 0x10000050 to 0x100000B8, which should not\
 be used by the application. Make sure the linker script does not place any variables on that\
 section! (Project properties -> C/C++ Build Settings -> MCU Settings -> Memory Details)
*/


// loop back mode connects CAN rx and tx lines internally together
// echoing all sent messages back to receiver.
#define CAN_LOOP_BACK_MODE	0


// upper-nibble values for CAN_ODENTRY.entrytype_len
#define OD_NONE 0x00 // Object Dictionary entry doesn't exist
#define OD_EXP_RO 0x10 // Object Dictionary entry expedited, read-only
#define OD_EXP_WO 0x20 // Object Dictionary entry expedited, write-only
#define OD_EXP_RW 0x30 // Object Dictionary entry expedited, read-write
#define OD_SEG_RO 0x40 // Object Dictionary entry segmented, read-only
#define OD_SEG_WO 0x50 // Object Dictionary entry segmented, write-only
#define OD_SEG_RW 0x60 // Object Dictionary entry segmented, read-write


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
	MSG_OBJ_16 = 16,
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


typedef enum {
	CANOPEN_TXSDO1 = 0,
	CANOPEN_RXSDO1,
	CANOPEN_TXSDO2,
	CANOPEN_RXSDO2,
	CANOPEN_TXSDO3,
	CANOPEN_RXSDO3,
	CANOPEN_SDO_COUNT
} canopen_sdo_e;


typedef enum {
	UW_DEVICE_STATUS_OFF = 0,
	UW_DEVICE_STATUS_PREOPERATIONAL,
	UW_DEVICE_STATUS_OPERATIONAL,
} uw_device_status_e;



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



/// @brief: CANopen object dictionary constant entry
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	uint16_t index;
	uint8_t subindex;
	uint8_t len;
	uint32_t val;
} hal_canopen_obj_dict_const_entry_st;



/// @brief: CANopen object dictionary entry
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	uint16_t index;
	uint8_t subindex;
	uint8_t entrytype_len;
	uint8_t *val;
} hal_canopen_obj_dict_entry_st;



/// brief: Defines CANopen PDO types
typedef enum {
	CANOPEN_TXPDO1 = 0,
	CANOPEN_TXPDO2,
	CANOPEN_TXPDO3,
	CANOPEN_TXPDO4,
	CANOPEN_RXPDO1,
	CANOPEN_RXPDO2,
	CANOPEN_RXPDO3,
	CANOPEN_RXPDO4,
	CANOPEN_PDO_COUNT
} hal_canopen_pdo_types_e;



/// @brief: describes CANopen message ID prefixes for CANopen message protocols
typedef enum {
	CANOPEN_NMT_ID = 0,
	CANOPEN_NMT_ERROR_ID = 0x700,
	CANOPEN_BOOTUP_ID = 0x700,
	CANOPEN_SYNC_ID = 0x80,
	CANOPEN_EMERGENCY_ID = 0x80,
	CANOPEN_TIME_STAMP_ID = 0x100,
	CANOPEN_TX_PDO1_ID = 0x180,
	CANOPEN_TX_PDO2_ID = 0x280,
	CANOPEN_TX_PDO3_ID = 0x380,
	CANOPEN_TX_DEBUGPDO_ID = 0x480,
	CANOPEN_RX_PDO1_ID = 0x200,
	CANOPEN_RX_PDO2_ID = 0x300,
	CANOPEN_RX_PDO3_ID = 0x400,
	CANOPEN_RX_DEBUGPDO_ID = 0500,
	CANOPEN_SDO_REQUEST_ID = 0x600,
	CANOPEN_SDO_REPPLY_ID = 0x580
} hal_canopen_message_type_ids_e;


/// @brief: Describes CANopen NMT message protocol commands. NMT message should be
/// 2 bytes long, where first byte indicates command and second byte target node.
typedef enum {
	CANOPEN_NMT_START_CMD = 0x1,
	CANOPEN_NMT_STOP_CMD = 0x2,
	CANOPEN_NMT_PREOP_CMD = 0x80,
	CANOPEN_NMT_RESET_CMD = 0x81,
	CANOPEN_NMT_RESET_COMMUNICATION_CMD = 0x82
} hal_canopen_nmt_messages_e;







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
void hal_can_register_rx_callback(void (*callback_function)(hal_can_msg_obj_st*));



/// @brief: Registers a callback function for CAN errors
/// Callback function gives received errors as parameter.
void hal_can_register_error_callback(void (*callback_function)(uint32_t));


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







/**************** CANopen ******************/

/// @brief: initializes CAN hardware as a CANopen node.
/// @pre: hal_init_can function should be called before this function
/// @param node_id CANopen node ID. 1 - 127 are acceptable
/// @param object_dictionary_const_entry_count the length of
/// 	object_dictionary_const_entries-array
/// @param object_dictionary_const_entries Array defining all read only entries in
///		CANopen object dictionary
/// @param object_dictionary_entry_count the length of object_dictionary_entries
/// @param object_dictionary_entries Array defining all variable and writable
///		CANopen object dictionary entries
///
/// @note: For HAL layer NMT protocol implementation, call hal_canopen_set_device_status_ptr
/// before calling this function.
void hal_canopen_init_node(uint8_t node_id,
		uint32_t object_dictionary_const_entry_count,
		hal_canopen_obj_dict_const_entry_st* object_dictionary_const_entries,
		uint32_t object_dictionary_entry_count,
		hal_canopen_obj_dict_entry_st* object_dictionary_entries);



/// @brief: Sets the data of a specific TxPDO.
/// @param pdo The number of the pdo which data is to be set (txpdo1 ... txpdo3)
/// @param index Starting byte index of the data to be written (0 ... 7)
/// @param data pointer to array of the data
/// @param data_length The length of the data in bytes
void hal_canopen_set_pdo_data(hal_canopen_pdo_types_e pdo,
		uint8_t index,
		uint8_t* data,
		uint8_t data_length);


/// @brief: Returns a pointer to PDO's data. Make sure not to over-index the
/// data array.
/// @param pdo The number of PDO desired. Should not be more or equal to CANOPEN_PDO_COUNT
uint8_t* hal_canopen_get_pdo_data(hal_canopen_pdo_types_e pdo);


/// @brief: registers a callback function to be called when PDO message has been received
/// One callback function is used for all RxPDO's. The function gives the received PDO
/// as a parameter.
void hal_canopen_register_pdo_callback(void (*callback_function)(hal_canopen_pdo_types_e pdo));

/// @brief: registers a callback function to be called when SDO expedited write call has happened
void hal_canopen_register_sdo_write_callback(void (*callback_function)
		(uint16_t index, uint8_t subindex, uint8_t* data));

/// @brief: Registers a callback function for received NMT messages.
/// HAL layer interface for NMT messages can be enabled by assigning device_status pointer
/// via hal_canopen_set_device_status_ptr before call to hal_canopen_init_node.
/// Callback needs to be registered only if application layer implements a
/// custom handling of the NMT messages.
///
/// @note: The return value of the callback function determines if the HAL layer should
/// handle the message the default way. Return value false disables NMT processing
/// in the HAL.
void hal_canopen_register_nmt_callback(bool (*callback_function)
		(hal_canopen_nmt_messages_e command));


/// @brief: Getter for CANopen device status
uw_device_status_e hal_canopen_get_device_status();

/// @brief: Setter for CANopen device status
///
/// @note: Should be called before hal_canopen_init_node!
void hal_canopen_set_device_status_ptr(uw_device_status_e* value_ptr);



/// @brief: Enumerated values for CANopen PDO transmit function return values
typedef enum {
	PDO_SENT_SUCCESFULLY = 0,
	PDO_DEVICE_STATUS_NULLPTR,
	PDO_DEVICE_STATUS_NOT_OPERATIONAL,
	PDO_INVALID_PDO_NUM,
	PDO_MSG_OBJ_PENDING
} hal_can_pdo_return_values_e;

/// @brief: Sends the CANopen tx pdo according to CANopen PDO standard asychronously.
/// Note that multiple calls of this function with the same PDO doesn't
/// quarantee multiple sent messages.
/// Note: Make sure to initialize PDO before sending.
/// @param pdo Number of pdo to be sent Enum defined on line 138.
/// @return Enum defining if sending succeeded
hal_can_pdo_return_values_e hal_canopen_send_pdo(hal_canopen_pdo_types_e pdo);


/// @brief: Sends the CANopen tx pdo according to CANopen PDO standard synchronously.
/// Function returns after the message has been succesfully sent.
///
/// @note: Interrupts have to be enabled for this to work! If this function is called
/// at the interrupt level, make sure that the CAN interrupt has higher priority than this.
///
/// @param pdo Number of pdo to be sent Enum defined on line 138.
/// @return true if message object was free and message could be sent, false otherwise.
bool hal_canopen_force_send_pdo(hal_canopen_pdo_types_e pdo);


/// @brief: Sends CANopen standard BOOT UP message.
/// Every CANopen node should send BOOT UP message when they have initialized themselves.
/// @param msg Pointer to statically allocated message data structure.
/// Messages message object should be set beforehand.
/// @return true if message object was free and message could be sent, false otherwise.
bool hal_canopen_send_boot_up_msg(hal_can_msg_obj_st* msg);



/// @brief: Sends CANopen NMT command message.
/// NMT commands should be sent only by bus'es master node
/// @param msg Pointer to statically allocated message data structure.
/// @param cmd NMT command to be sent
/// @param target_node_id Node ID of the device where message should be sent. 0 means broadcast.
/// @return true if message object was free and message could be sent, false otherwise.
bool hal_canopen_send_nmt_command(hal_can_msg_obj_st* msg, hal_canopen_nmt_messages_e cmd,
		uint8_t target_node_id);



/// @brief: Sends CANopen SDO read request command.
/// @param msg Pointer to statically allocated message data structure.
/// @param target_node_id Node ID of the device where message should be sent. 0 means broadcast.
/// @param index Target node's object dictionary index requested to be read.
/// @param sub-index Target node's object dictionary sub-index requested to be read.
/// @return true if message object was free and message could be sent, false otherwise.
bool hal_canopen_send_sdo_read_request(hal_can_msg_obj_st* msg,
		uint8_t target_node_id,
		uint16_t index,
		uint8_t sub_index);

/// @brief: Sends CANopen SDO write request command.
/// @param msg Pointer to statically allocated message data structure.
/// @param target_node_id Node ID of the device where message should be sent. 0 means broadcast.
/// @param index Target node's object dictionary index requested to be written.
/// @param sub-index Target node's object dictionary sub-index requested to be written.
/// @param data pointer to data to be sent
/// @param data_length length of data to be sent in bytes
/// @return true if message object was free and message could be sent, false otherwise.
bool hal_canopen_send_sdo_write_request(hal_can_msg_obj_st* msg,
		uint8_t target_node_id,
		uint16_t index,
		uint8_t sub_index,
		uint8_t* data,
		uint8_t data_length);



/// @brief: The base address from which forward the Usewood CAN debug interface will use CAN message ID's
/// ID is defined as this base address + CANopen node_id
#define hal_can_debug_rx_msg_id_base		0xffff
#define hal_can_debug_tx_msg_id_base		0xffffff


#endif /* HAL_CAN_CONTROLLER_H_ */
