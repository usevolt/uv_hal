/*
 * uw_canopen.h
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */

#ifndef UW_CANOPEN_H_
#define UW_CANOPEN_H_

#include "uw_can.h"

/// @file: A software CANopen protocol implementation


/// @brief: Mask for CAN_ID field's node_id bits
#define UW_CANOPEN_NODE_ID_MASK		0x7F

typedef enum {
	UW_CANOPEN_UNKNOWN_PROTOCOL,
	UW_CANOPEN_SDO_REQUEST_ID = 	0x600,
	UW_CANOPEN_SDO_RESPONSE_ID = 	0x580,
	UW_CANOPEN_TXPDO1_ID = 			0x180,
	UW_CANOPEN_TXPDO2_ID = 			0x280,
	UW_CANOPEN_TXPDO3_ID = 			0x380,
	UW_CANOPEN_TXPDO4_ID = 			0x480,
	UW_CANOPEN_RXPDO1_ID = 			0x200,
	UW_CANOPEN_RXPDO2_ID = 			0x300,
	UW_CANOPEN_RXPDO3_ID = 			0x400,
	UW_CANOPEN_RXPDO4_ID =			0x500,
	UW_CANOPEN_BOOTUP_ID = 			0x700,
	UW_CANOPEN_HEARTBEAT_ID = 		0x700,
	UW_CANOPEN_NMT_ID = 			0x0
} uw_canopen_protocol_ids_e;

typedef enum {
	UW_CANOPEN_NO_ERROR = 0,
	UW_CANOPEN_ERROR_UNKNOWN_PROTOCOL,
	UW_CANOPEN_UNIMPLEMENTED_PROTOCOL,
	UW_CANOPEN_CORRUPTION_ERROR
} uw_canopen_errors_e;


typedef enum {
	UW_CANOPEN_STATE_BOOT_UP =			0x0,
	UW_CANOPEN_STATE_STOPPED = 			0x4,
	UW_CANOPEN_STATE_OPERATIONAL =		0x5,
	UW_CANOPEN_STATE_PREOPERATIONAL =	0x7F
} uw_canopen_node_states_e;



typedef struct {
	/// @brief: maximum of 8 Message data bytes
	union {
		uint8_t data_8bit[8];
		uint16_t data_16bit[4];
		uint32_t data_32bit[2];
		uint64_t data_64bit;
	};
	uint8_t data_length;
} uw_pdo_message_st;

typedef enum {
	UW_RO = 1,
	UW_RW,
	UW_WO
} uw_permissions_e;


typedef struct {
	/// @brief: Index for this CANopen object dictionary entry
	uint16_t index;
	/// @brief: Subindex for this CANopen object dictionary entry
	uint8_t subindex;
	/// @brief: Request regarding this sdo entry, as a uw_canopen_sdo_commands_e
	uint8_t request;
	/// @brief: Data length for this CANopen object dictionary entry.
	/// @note: Currently only expedited entries are supported => this has to be betweeen 1 to 4.
	uint8_t data_length;
	/// @brief: Data of this CANopen object dictionary entry
	union {
		uint8_t data_8bit[4];
		uint16_t data_16bit[2];
		uint32_t data_32bit;
	};

} uw_canopen_sdo_message_st;


typedef enum {
	UW_SDO_ERROR_COMMAND_NOT_VALID = 					0x05040001,
	UW_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 		0x06010001,
	UW_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	UW_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	UW_SDO_ERROR_OBJECT_DOES_NOT_EXIST = 				0x06020000,
	UW_SDO_ERROR_SUBINDEX_DOES_NOT_EXIST = 				0x06090011,
	UW_SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	UW_SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 			0x06090032,
	UW_SDO_ERROR_GENERAL = 								0x08000000
} uw_sdo_error_codes_e;

typedef enum {
	UW_SDO_CMD_READ = 					0x40,
	UW_SDO_CMD_READ_RESPONSE_1_BYTE =	0x4F,
	UW_SDO_CMD_READ_RESPONSE_2_BYTES =	0x4B,
	UW_SDO_CMD_READ_RESPONSE_4_BYTES =	0x43,
	UW_SDO_CMD_READ_RESPONSE_BYTES =	0x42,
	UW_SDO_CMD_WRITE_1_BYTE = 			0x2F,
	UW_SDO_CMD_WRITE_2_BYTES = 			0x2B,
	UW_SDO_CMD_WRITE_4_BYTES = 			0x23,
	UW_SDO_CMD_WRITE_BYTES = 			0x22,
	UW_SDO_CMD_WRITE_RESPONSE =			0x60,
	UW_SDO_CMD_ERROR = 					0x80
} uw_canopen_sdo_commands_e;


/// @brief: Struct for general CANopen message. This includes all errors, messsage types, as well as the
// message itself.
typedef struct {
	/// @brief: Tells the type of this message. This specifies what errors and message structures are
	/// stored in the unions.
	uw_canopen_protocol_ids_e type;
	/// @brief: Stores the CANopen node_id of the message
	uint8_t node_id;
	/// @brief: Error fields. As different CAopen protocols have different errors, this is defined as a union.
	union {
		uw_canopen_errors_e general;
		uw_sdo_error_codes_e sdo;
	} errors;
	/// @brief: This holds the message data.
	/// Refer to the message type to fetch the right message type
	union {
		uw_canopen_sdo_message_st sdo;
		uw_pdo_message_st pdo;
		uint8_t nmt;
		union {
			uint8_t heartbeat;
			uint8_t boot_up;
		};
	} msg;
} uw_canopen_msg_st;



//************ OBJECT DICTIONARY **************
typedef struct {
	/// @brief: Index for this CANopen object dictionary entry
	uint16_t index;
	/// @brief: Subindex for this CANopen object dictionary entry
	uint8_t subindex;
	/// @brief: Data length for this CANopen object dictionary entry.
	/// @note: Currently only expedited entries are supported => this has to be betweeen 1 to 4.
	uint8_t data_length;
	/// @brief: Data of this CANopen object dictionary entry
	union {
		uint8_t data_8bit[4];
		uint16_t data_16bit[2];
		uint32_t data_32bit;
	};
	/// @brief: Permissions for this CANopen object dictionary entry
	uw_permissions_e permissions;
	/// @brief: Callback function which is executed when this entry was written via CAN-bus
	/// @param data: The new data which was written.
	/// @param node_id: The node_id of the device who sent the write request
	void (*write_callback)(uint8_t data[4], uint8_t node_id);
} uw_canopen_obj_dict_entry_st;



/// @brief: parses the CAN message and returns the CANopen protocol message from it
/// @return CANopen message structure
uw_canopen_msg_st uw_canopen_parse_message(uw_can_message_st* message);


/// @brief: The CANopen step function. Makes sure that the txPDO and heartbeat messages
/// are sent cyclically in a right time step
void uw_canopen_step(unsigned int step_ms);



/// @brief: Initializes the CANopen node and the object dictionary
/// @param obj dict: Pointer to the uw_canopen_obj_dict_entry_st array. This will be used as the location
/// where object dictionary entries are modified.
/// @param obj_dict_length: The length of the object dictionary, e.g. the number of different indexes.
void uw_canopen_init(uw_canopen_obj_dict_entry_st* obj_dict, unsigned int obj_dict_length);



/**** MANUAL INTERFACE *****/

/// @brief: Used to set the device state. Device will start in UW_CANOPEN_BOOT_UP state
/// and it should move itself to pre-operational state arfter boot up is done.
void uw_canopen_set_state(uw_canopen_node_states_e state);



#endif /* UW_CANOPEN_H_ */


