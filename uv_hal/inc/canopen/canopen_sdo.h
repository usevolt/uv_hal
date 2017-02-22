/*
 * canopen_sdo.h
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_SDO_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_SDO_H_



#include <uv_hal_config.h>
#include "canopen/canopen_common.h"

#if CONFIG_CANOPEN

typedef enum {
	CANOPEN_UNKNOWN_PROTOCOL,
	CANOPEN_SDO_REQUEST_ID = 		0x600,
	CANOPEN_SDO_RESPONSE_ID = 		0x580,
	CANOPEN_SDO_ERROR_ID =			0x80,
	CANOPEN_EMCY_ID =				0x80,
	CANOPEN_TXPDO1_ID = 			0x180,
	CANOPEN_TXPDO2_ID = 			0x280,
	CANOPEN_TXPDO3_ID = 			0x380,
	CANOPEN_TXPDO4_ID = 			0x480,
	CANOPEN_RXPDO1_ID = 			0x200,
	CANOPEN_RXPDO2_ID = 			0x300,
	CANOPEN_RXPDO3_ID = 			0x400,
	CANOPEN_RXPDO4_ID = 			0x500,
	CANOPEN_BOOTUP_ID = 			0x700,
	CANOPEN_HEARTBEAT_ID = 			0x700,
	CANOPEN_NMT_ID = 				0x0
} uv_canopen_protocol_ids_e;




typedef enum {
	CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND =					0x05040001,
	CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 			0x06010000,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST = 					0x06020000,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 				0x06090032,
	CANOPEN_SDO_ERROR_GENERAL = 								0x08000000
} _uv_sdo_error_codes_e;
typedef uint32_t uv_sdo_error_codes_e;




/// @brief: SDO command specifier, e.g. the MSB data byte of the CAN message
typedef enum {
	CANOPEN_SDO_CMD_READ = 						0x40,
	CANOPEN_SDO_CMD_READ_RESPONSE_1_BYTE =		0x4F,
	CANOPEN_SDO_CMD_READ_RESPONSE_2_BYTES =		0x4B,
	CANOPEN_SDO_CMD_READ_RESPONSE_4_BYTES =		0x43,
	CANOPEN_SDO_CMD_READ_RESPONSE_BYTES =		0x42,
	CANOPEN_SDO_CMD_WRITE_1_BYTE = 				0x2F,
	CANOPEN_SDO_CMD_WRITE_2_BYTES = 			0x2B,
	CANOPEN_SDO_CMD_WRITE_4_BYTES = 			0x23,
	CANOPEN_SDO_CMD_WRITE_BYTES = 				0x22,
	CANOPEN_SDO_CMD_WRITE_RESPONSE =			0x60,
	CANOPEN_SDO_CMD_ERROR = 					0x80
} _uv_canopen_sdo_commands_e;
typedef uint8_t uv_canopen_sdo_commands_e;


/// @brief: Defines the indexspaces for CANopen object main indexes.
/// 0x1000 - 0x1FFF objects are reserved for CANopen communication parameters
/// and many of them are defined in CiA documents.
/// 0x2000 - 0x2FFF is mainly for application objects.
typedef enum {
	CANOPEN_SDO_COMMUNICATION_INDEXSPACE =		0x1000,
	CANOPEN_SDO_APPLICATION_INDEXSPACE =		0x2000
} uv_sdo_mindex_namespaces_e;



typedef struct {
	/// @brief: Request regarding this sdo entry, as a uv_canopen_sdo_commands_e
	uv_canopen_sdo_commands_e request;
	/// @brief: Index for this CANopen object dictionary entry
	uint16_t main_index;
	/// @brief: Subindex for this CANopen object dictionary entry
	uint8_t sub_index;
	/// @brief: Data length for this CANopen object dictionary entry.
	/// @note: Currently only expedited entries are supported => this has to be betweeen 1 to 4.
	uint8_t data_length;
	/// @brief: Data of this CANopen object dictionary entry
	union {
		uint8_t data_8bit[4];
		uint16_t data_16bit[2];
		uint32_t data_32bit;
	};

} uv_canopen_sdo_message_st;




typedef struct {

} _canopen_sdo_st;



#endif /* UV_HAL_INC_CANOPEN_CANOPEN_SDO_H_ */
#endif
