/*
 * canopen_sdo_client.h
 *
 *  Created on: Aug 27, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_SDO_CLIENT_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_SDO_CLIENT_H_


#include <uv_hal_config.h>
#include "canopen/canopen_common.h"
#include "uv_can.h"
#include "canopen/canopen_sdo.h"


#if CONFIG_CANOPEN


void _uv_canopen_sdo_client_init(void);

void _uv_canopen_sdo_client_reset(void);

void _uv_canopen_sdo_client_step(uint16_t step_ms);

void _uv_canopen_sdo_client_rx(const uv_can_message_st *msg,
		sdo_request_type_e sdo_type, uint8_t node_id);

/// @brief: Sends a CANOpen SDO write request and waits for the reply as confirmation
uv_errors_e _uv_canopen_sdo_client_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data);


/// @brief: Sends a CANOpen SDO read request and waits for the response
/// **timeout_ms** milliseconds. If the read request failed or the timeout
/// expires, returns an error.
uv_errors_e _uv_canopen_sdo_client_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data);


#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER

/// @brief: Sends CANOpen SDO block write request and returns after the
/// transfer is finished or an error is received.
uv_errors_e _uv_canopen_sdo_client_block_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data);


/// @brief: Sends a CANOpen SDO block read request and returns after the
/// transfer is finished or an error is received.
uv_errors_e _uv_canopen_sdo_client_block_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data);

#endif

#endif


#endif /* UV_HAL_INC_CANOPEN_CANOPEN_SDO_CLIENT_H_ */
