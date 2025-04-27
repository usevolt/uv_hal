/*
 * uv_j1939.h
 *
 *  Created on: May 5, 2023
 *      Author: usevolt
 */

#ifndef HAL_UV_HAL_INC_UV_J1939_H_
#define HAL_UV_HAL_INC_UV_J1939_H_

#include "uv_can.h"

/// @file Defines for SAE J1939 protocol


#define UV_J1939_PGN_DM1			(0xFECA)

#define UV_J1939_PGN_MASK			(0x3FFFF00)


/// @brief: Returns the parameter group number from the message. 0 in case of error.
static inline uint16_t uv_j1939_get_pgn(uv_can_msg_st *msg) {
	return (msg->type == CAN_EXT) ? ((msg->id & 0xFFFF00) >> 8) : 0;
}


static inline uint8_t uv_j1939_get_source_address(uv_can_msg_st *msg) {
	return (msg->id & 0xFF);
}


#endif /* HAL_UV_HAL_INC_UV_J1939_H_ */
