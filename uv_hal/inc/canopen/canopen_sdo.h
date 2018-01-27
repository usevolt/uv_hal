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
#include "uv_can.h"

#if CONFIG_CANOPEN



typedef enum {
	CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED =				0x05030000,
	CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT =					0x05040000,
	CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND =					0x05040001,
	CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 			0x06010000,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST = 					0x06020000,
	CANOPEN_SDO_ERROR_OBJECT_CANNOT_BE_MAPPED_TO_PDO = 			0x06040041,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 				0x06090032,
	CANOPEN_SDO_ERROR_OBJECT_ACCESS_FAILED_DUE_TO_HARDWARE =	0x06060000,
	CANOPEN_SDO_ERROR_GENERAL = 								0x08000000
} _uv_sdo_error_codes_e;
typedef uint32_t uv_sdo_error_codes_e;





enum {
	CANOPEN_SDO_STATE_READY = 1,
	CANOPEN_SDO_STATE_TRANSFER_ABORTED,
	CANOPEN_SDO_STATE_TRANSFER_DONE,
	CANOPEN_SDO_STATE_EXPEDITED_DOWNLOAD,
	CANOPEN_SDO_STATE_EXPEDITED_UPLOAD,
	CANOPEN_SDO_STATE_SEGMENTED_UPLOAD,
	CANOPEN_SDO_STATE_SEGMENTED_UPLOAD_WFR,
	CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD,
	CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD_WFR
};
typedef uint8_t canopen_sdo_state_e;


enum {
	INVALID_MSG = 0,
	UNKNOWN_SDO_MSG = 0xFF,
	ABORT_DOMAIN_TRANSFER = 0b10000000,
	INITIATE_DOMAIN_DOWNLOAD = 0b00100000,
	INITIATE_DOMAIN_DOWNLOAD_REPLY = 0b01100000,
	DOWNLOAD_DOMAIN_SEGMENT = 0b0000000,
	DOWNLOAD_DOMAIN_SEGMENT_REPLY = 0b00100000,
	INITIATE_DOMAIN_UPLOAD = 0b01000000,
	UPLOAD_DOMAIN_SEGMENT = 0b01100000,
	UPLOAD_DOMAIN_SEGMENT_REPLY = 0b00000000
};
typedef uint8_t sdo_request_type_e;



void _uv_canopen_sdo_init(void);


extern void _uv_canopen_sdo_client_reset(void);
extern void _uv_canopen_sdo_server_reset(void);

static inline void _uv_canopen_sdo_reset(void) {
	_uv_canopen_sdo_client_reset();
	_uv_canopen_sdo_server_reset();
}

void _uv_canopen_sdo_client_step(uint16_t step_ms);
void _uv_canopen_sdo_server_step(uint16_t step_ms);

static inline void _uv_canopen_sdo_step(uint16_t step_ms) {
	_uv_canopen_sdo_client_step(step_ms);
	_uv_canopen_sdo_server_step(step_ms);
}



void _uv_canopen_sdo_rx(const uv_can_message_st *msg);


/// @brief: Sends a CANOpen SDO abort message
void _uv_canopen_sdo_abort(uint16_t request_response, uint16_t main_index,
		uint8_t sub_index, uv_sdo_error_codes_e err_code);



/// @brief: Finds the object dictionary object. Used by canopen_sdo_client and server modules
bool _canopen_find_object(const uv_can_message_st *msg,
		canopen_object_st *obj, canopen_permissions_e permission_req);


/// @brief: Copies canopen object data to message
///
/// @param dest: Pointer to can message where data is written
/// @param src: Pointer to canopen object where data is read
/// @param subindex: The original request message's subindex field. This is used for
/// indexing array and string type data
void _canopen_copy_data(uv_can_message_st *dest,
		const canopen_object_st *src, uint8_t subindex);


/// @brief: Writes canopen object data from message to object
///
/// @return: True if succeess, false if unsupported access to object
///
/// @param dest: Pointer to canopen object where data is written
/// @param src: Pointer to can message from where the data is read
/// @param subindex: The original request message's subindex field. This is used for
/// indexing array and string type data
bool _canopen_write_data(canopen_object_st *dest,
		const uv_can_msg_st *src, uint8_t subindex);


#endif /* UV_HAL_INC_CANOPEN_CANOPEN_SDO_H_ */
#endif
