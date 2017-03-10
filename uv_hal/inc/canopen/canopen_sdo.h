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
	CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND =					0x05040001,
	CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT = 			0x06010000,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT = 	0x06010001,
	CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT = 	0x06010002,
	CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST = 					0x06020000,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH = 			0x06090031,
	CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW = 				0x06090032,
	CANOPEN_SDO_ERROR_OBJECT_ACCESS_FAILED_DUE_TO_HARDWARE =	0x06060000,
	CANOPEN_SDO_ERROR_GENERAL = 								0x08000000
} _uv_sdo_error_codes_e;
typedef uint32_t uv_sdo_error_codes_e;




/// @brief: Defines the indexspaces for CANopen object main indexes.
/// 0x1000 - 0x1FFF objects are reserved for CANopen communication parameters
/// and many of them are defined in CiA documents.
/// 0x2000 - 0x2FFF is mainly for application objects.
typedef enum {
	CANOPEN_SDO_COMMUNICATION_INDEXSPACE =		0x1000,
	CANOPEN_SDO_APPLICATION_INDEXSPACE =		0x2000
} uv_sdo_mindex_namespaces_e;



enum {
	CANOPEN_SDO_STATE_READY = 1,
	CANOPEN_SDO_STATE_TRANSFER_ABORTED = 2,
	CANOPEN_SDO_STATE_TRANSFER_DONE = 3,
	CANOPEN_SDO_STATE_EXPEDITED_WRITE = (1 << 4),
	CANOPEN_SDO_STATE_EXPEDITED_READ = (2 << 4),
	CANOPEN_SDO_STATE_SEGMENTED_READ = (3 << 4),
	CANOPEN_SDO_STATE_SEGMENTED_WRITE = (4 << 4)
};
typedef uint8_t canopen_sdo_state_e;
#define CANOPEN_SDO_STATE_FINISHED_MASK	(0b1111)


void _uv_canopen_sdo_init(void);

void _uv_canopen_sdo_reset(void);

void _uv_canopen_sdo_step(uint16_t step_ms);

void _uv_canopen_sdo_rx(const uv_can_message_st *msg);

/// @brief: Sends a CANOpen SDO write request without waiting for the response
uv_errors_e _uv_canopen_sdo_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data);


#if CONFIG_CANOPEN_SDO_SYNC
/// @brief: Sends a CANOpen SDO write request and waits for the response
/// **timeout_ms** milliseconds. If the write request failed or the timeout
/// expires, returns an error.
uv_errors_e _uv_canopen_sdo_write_sync(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len, void *data, int32_t timeout_ms);

/// @brief: Sends a CANOpen SDO read request and waits for the response
/// **timeout_ms** milliseconds. If the read request failed or the timeout
/// expires, returns an error.
uv_errors_e _uv_canopen_sdo_read_sync(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data, int32_t timeout_ms);
#endif


/// @brief: Sends a CANOpen SDO abort message
void _uv_canopen_sdo_abort(uint16_t request_response, uint16_t main_index,
		uint8_t sub_index, uv_sdo_error_codes_e err_code);



#endif /* UV_HAL_INC_CANOPEN_CANOPEN_SDO_H_ */
#endif
