/*
 * canopen_sdo_server.h
 *
 *  Created on: Aug 27, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_SDO_SERVER_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_SDO_SERVER_H_

#include <uv_hal_config.h>
#include "canopen/canopen_common.h"
#include "uv_can.h"
#include "canopen/canopen_sdo.h"

#if CONFIG_CANOPEN



void _uv_canopen_sdo_server_init(void);

void _uv_canopen_sdo_server_reset(void);

void _uv_canopen_sdo_server_step(uint16_t step_ms);

void _uv_canopen_sdo_server_rx(const uv_can_message_st *msg, sdo_request_type_e sdo_type);

void _uv_canopen_sdo_server_add_callb(void (*callb)(uint16_t mindex, uint8_t sindex));



#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_SDO_SERVER_H_ */
