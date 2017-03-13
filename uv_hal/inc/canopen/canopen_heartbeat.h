/*
 * canopen_heartbeat.h
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_



#include <uv_hal_config.h>
#include "uv_can.h"
#include "canopen/canopen_common.h"

#if CONFIG_CANOPEN





void _uv_canopen_heartbeat_init(void);

void _uv_canopen_heartbeat_reset(void);

void _uv_canopen_heartbeat_step(uint16_t step_ms);

void _uv_canopen_heartbeat_rx(const uv_can_message_st *msg);


#endif /* UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_ */
#endif
