/*
 * canopen_nmt.h
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_NMT_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_NMT_H_

#include <uv_hal_config.h>
#include "canopen/canopen_common.h"
#include "uv_can.h"


#if CONFIG_CANOPEN

typedef enum {
	/// @brief: CANopen stack is in boot up state until
	/// it has been initialized with uv_canopen_init
	CANOPEN_BOOT_UP =			0x0,
	/// @brief: CANopen device is stopped. It can only receive NMT and
	/// heartbeat messages.
	CANOPEN_STOPPED = 			0x4,
	/// @brief: In operational state even PDO messages can be sent
	/// and transmitted.
	CANOPEN_OPERATIONAL =		0x5,
	/// @brief: Configuration state where PDO messages are not sent.
	/// The device can be configured with SDO messages.
	/// The device ends up to this state after a call to uv_canopen_init.
	CANOPEN_PREOPERATIONAL =	0x7F
} canopen_node_states_e;


/// @brief: Describes all possible NMT commands
typedef enum {
	CANOPEN_NMT_START_NODE 			= 0x1,
	CANOPEN_NMT_STOP_NODE 			= 0x2,
	CANOPEN_NMT_SET_PREOPERATIONAL 	= 0x80,
	CANOPEN_NMT_RESET_NODE 			= 0x81,
	CANOPEN_NMT_RESET_COM 			= 0x82
} canopen_nmt_commands_e;




void _uv_canopen_nmt_init(void);

void _uv_canopen_nmt_reset(void);

void _uv_canopen_nmt_step(uint16_t step_ms);

void _uv_canopen_nmt_rx(const uv_can_message_st *msg);

canopen_node_states_e _uv_canopen_nmt_get_state(void);

void _uv_canopen_nmt_set_state(canopen_node_states_e state);


#if CONFIG_CANOPEN_NMT_MASTER

/// @brief: Resets the specified CANopen node. All nodes can be reset by giving **nodeid** as 0.
void uv_canopen_nmt_master_reset_node(uint8_t nodeid);

/// @brief: Sets the node state to **state**
void uv_canopen_nmt_master_set_node_state(uint8_t nodeid, canopen_nmt_commands_e state);

#endif


#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_NMT_H_ */
