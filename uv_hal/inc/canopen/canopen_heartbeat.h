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
#include "canopen/canopen_nmt.h"

#if CONFIG_CANOPEN



#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
/// @brief: Returns true if the heartbeat producer indicated by **node_id**
/// has been expired, i.e. no heartbeat messages has been received from that node
/// in time specified in index [1016].
bool uv_canopen_heartbeat_producer_is_expired(uint8_t node_id);

canopen_node_states_e uv_canopen_heartbeat_producer_get_state(uint8_t nodeid);
#endif

void _uv_canopen_heartbeat_init(void);

void _uv_canopen_heartbeat_reset(void);

void _uv_canopen_heartbeat_step(uint16_t step_ms);

void _uv_canopen_heartbeat_rx(const uv_can_message_st *msg);


#endif /* UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_ */
#endif
